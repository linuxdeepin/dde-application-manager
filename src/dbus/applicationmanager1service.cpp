// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "applicationadaptor.h"
#include "applicationHooks.h"
#include "applicationchecker.h"
#include "applicationservice.h"
#include "dbus/AMobjectmanager1adaptor.h"
#include "dbus/applicationmanager1adaptor.h"
#include "desktopfilegenerator.h"
#include "global.h"
#include "propertiesForwarder.h"
#include "systemdsignaldispatcher.h"
#include <DUtil>
#include <QDBusMessage>
#include <QDirIterator>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QLoggingCategory>
#include <QStringBuilder>
#include <unistd.h>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(DDEAM, "dde.am.manager")

namespace {
template <typename Adaptor>
void setAdaptorAutoRelaySignals(Adaptor *adaptor, bool enabled) noexcept
{
    struct Accessor final : Adaptor
    {
        using Adaptor::setAutoRelaySignals;
    };

    if (adaptor != nullptr) {
        static_cast<Accessor *>(adaptor)->setAutoRelaySignals(enabled);
    }
}

void sendObjectManagerSignal(const QString &path, const char *member, const QDBusObjectPath &objectPath, const QVariant &payload)
{
    auto msg = QDBusMessage::createSignal(path, fromStaticRaw(ObjectManagerInterface), member);
    msg << objectPath;
    msg << payload;
    ApplicationManager1DBus::instance().globalServerBus().send(msg);
}

struct ParsedAutostartEntry
{
    DesktopFile desktopFile;
    DesktopEntry entry;
};

QString desktopIdFromRelativePath(QStringView relativePath) noexcept
{
    if (!relativePath.endsWith(desktopSuffix)) {
        return {};
    }

    auto id = relativePath.chopped(desktopSuffix.size()).toString();
    id.replace(QDir::separator(), u'-');
    return id;
}

template <typename T>
void forEachApplicationDesktopFile(T &&func) noexcept
{
    static_assert(std::is_invocable_v<T, DesktopFile>,
                  "application desktop iterator callback should accept one DesktopFile argument");

    QSet<QString> seenDesktopIds;
    for (const auto &dirPath : getApplicationsDirs()) {
        const QFileInfo dirInfo{dirPath};
        if (!dirInfo.isDir()) {
            continue;
        }

        QDirIterator it{
            dirPath, {u"*.desktop"_s}, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::Subdirectories};
        while (it.hasNext()) {
            const auto info = it.nextFileInfo();
            const auto relativePath = QDir{dirPath}.relativeFilePath(info.absoluteFilePath());
            auto ret = DesktopFile::createDesktopFile(info, desktopIdFromRelativePath(relativePath));
            if (!ret) {
                continue;
            }

            auto file = std::move(ret).value();
            if (file.desktopId().isEmpty() || seenDesktopIds.contains(file.desktopId())) {
                continue;
            }

            seenDesktopIds.insert(file.desktopId());
            if (func(std::move(file))) {
                return;
            }
        }
    }
}

template <typename T>
void forEachAutostartDesktopFile(T &&func) noexcept
{
    static_assert(std::is_invocable_v<T, DesktopFile>,
                  "autostart desktop iterator callback should accept one DesktopFile argument");

    QSet<QString> seenFileNames;
    for (const auto &dirPath : getAutoStartDirs()) {
        const QDir dir{dirPath};
        if (!dir.exists()) {
            continue;
        }

        const auto infoList = dir.entryInfoList({u"*.desktop"_s}, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name);
        for (const auto &info : infoList) {
            if (seenFileNames.contains(info.fileName())) {
                continue;
            }

            auto ret = DesktopFile::createDesktopFile(info, desktopIdFromRelativePath(info.fileName()));
            if (!ret) {
                continue;
            }

            seenFileNames.insert(info.fileName());
            if (func(std::move(ret).value())) {
                return;
            }
        }
    }
}

[[nodiscard]] std::optional<ParsedAutostartEntry> parseAutostartDesktopFile(DesktopFile desktopFile) noexcept
{
    DesktopFileGuard guard{desktopFile};
    if (!guard.try_open()) {
        return std::nullopt;
    }

    DesktopEntry entry;
    auto &file = desktopFile.sourceFileRef();
    auto err = entry.parse(file);
    if (err != ParserError::NoError) {
        qWarning() << "parse autostart file" << desktopFile.sourcePath() << "error:" << err;
        return std::nullopt;
    }

    if (ApplicationFilter::tryExecCheck(entry) || ApplicationFilter::showInCheck(entry) || ApplicationFilter::hiddenCheck(entry)) {
        qInfo() << "autostart application" << desktopFile.desktopId() << "couldn't pass check.";
        return std::nullopt;
    }

    guard.close();
    return ParsedAutostartEntry{std::move(desktopFile), std::move(entry)};
}
}  // namespace

ApplicationManager1Service::~ApplicationManager1Service() = default;

ApplicationManager1Service::ApplicationManager1Service(std::unique_ptr<Identifier> ptr,
                                                       std::weak_ptr<ApplicationManager1Storage> storage) noexcept
    : m_identifier(std::move(ptr))
    , m_storage(std::move(storage))
{
    // Initialize prelaunch splash helper only when running on Wayland.
    bool isWayland = false;
    if (auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        if (app->nativeInterface<QNativeInterface::QWaylandApplication>() != nullptr) {
            isWayland = true;
        }
    }

    if (isWayland) {
        m_splashHelper.reset(new (std::nothrow) PrelaunchSplashHelper());
        if (!m_splashHelper) {
            qCWarning(amPrelaunchSplash) << "Failed to allocate PrelaunchSplashHelper.";
        } else {
            qCInfo(amPrelaunchSplash) << "PrelaunchSplashHelper initialized.";
        }
    } else {
        qCInfo(amPrelaunchSplash) << "Skip PrelaunchSplashHelper (not running on Wayland)";
    }

    m_reloadTimer.setInterval(500);
    m_reloadTimer.setSingleShot(true);
    connect(&m_reloadTimer, &QTimer::timeout, this, &ApplicationManager1Service::doReloadApplications);
}

void ApplicationManager1Service::initService(QDBusConnection &connection) noexcept
{
    if (auto *tmp = new (std::nothrow) ApplicationManager1Adaptor{this}; tmp == nullptr) {
        qCCritical(DDEAM) << "new Application Manager Adaptor failed.";
        std::terminate();
    } else {
        setAdaptorAutoRelaySignals(tmp, false);
    }

    if (auto *tmp = new (std::nothrow) AMObjectManagerAdaptor{this}; tmp == nullptr) {
        qCCritical(DDEAM) << "new Object Manager of Application Manager Adaptor failed.";
        std::terminate();
    } else {
        setAdaptorAutoRelaySignals(tmp, false);
    }

    if (!registerObjectToDBus(
            this, fromStaticRaw(DDEApplicationManager1ObjectPath), fromStaticRaw(ApplicationManager1Interface))) {
        std::terminate();
    }

    if (m_jobManager.reset(new (std::nothrow) JobManager1Service(this)); !m_jobManager) {
        qCCritical(DDEAM) << "new JobManager failed.";
        std::terminate();
    }

    if (m_mimeManager.reset(new (std::nothrow) MimeManager1Service(this)); !m_mimeManager) {
        qCCritical(DDEAM) << "new MimeManager failed.";
        std::terminate();
    }

    if (m_compatibilityManager.reset(new (std::nothrow) CompatibilityManager()); !m_compatibilityManager) {
        qWarning() << "new CompatibilityManager failed.";
    }

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &ApplicationManager1Service::ReloadApplications);

    // Ensure all directories exist before adding watches
    const auto &userApp = getUserApplicationDir();
    if (!QDir{userApp}.mkpath(u"."_s)) {
        qCCritical(DDEAM) << "Failed to create directory:" << userApp;
        std::terminate();
    }

    const auto &desktopDirs = getApplicationsDirs();
    auto unhandled = m_watcher.addPaths(desktopDirs);
    for (const auto &dir : std::as_const(unhandled)) {
        qCCritical(DDEAM) << "couldn't watch directory:" << dir;
    }

    auto &dispatcher = SystemdSignalDispatcher::instance();

    connect(&dispatcher,
            &SystemdSignalDispatcher::SystemdUnitNew,
            this,
            qOverload<const QString &, const QDBusObjectPath &>(&ApplicationManager1Service::addInstanceToApplication));
    connect(&dispatcher,
            &SystemdSignalDispatcher::SystemdJobNew,
            this,
            [this](const QString &unitName, const QDBusObjectPath &systemdUnitPath) {
                auto info = processUnitName(unitName);
                if (!info) {
                    return;
                }

                if (info->applicationID.isEmpty()) {
                    return;
                }

                auto app = m_applicationList.value(info->applicationID);

                if (!app) {
                    return;
                }

                // 服务在 AM 之后启动那么 instance size 是 0， newJob 时尝试添加一次
                // 比如 dde-file-manager.service 如果启动的比 AM 晚，那么在 scanInstances 时不会 addInstanceToApplication
                if (!app->instances().empty()) {
                    return;
                }

                qCDebug(DDEAM) << "add Instance " << unitName << "on JobNew, " << app->instances().size();

                addInstanceToApplication(std::move(info).value(), systemdUnitPath);
            });

    connect(&dispatcher,
            &SystemdSignalDispatcher::SystemdUnitRemoved,
            this,
            &ApplicationManager1Service::removeInstanceFromApplication);

    auto envToPath = [this](const QStringList &envs) {
        auto path = std::find_if(envs.cbegin(), envs.cend(), [](QStringView env) { return env.startsWith(u"PATH="); });
        if (path == envs.cend()) {
            return;
        }

        auto pathView = QStringView{*path}.sliced(5);
        auto tokens = qTokenize(pathView, u':', Qt::SkipEmptyParts);
        m_systemdPathEnv.clear();

        for (auto view : tokens) {
            m_systemdPathEnv.append(view.toString());
        }
    };

    connect(&dispatcher, &SystemdSignalDispatcher::SystemdEnvironmentChanged, envToPath);

    auto &con = ApplicationManager1DBus::instance().globalDestBus();
    auto envMsg = QDBusMessage::createMethodCall(
        SystemdService, SystemdObjectPath, fromStaticRaw(SystemdPropInterfaceName), fromStaticRaw(SystemdGet));
    envMsg.setArguments({SystemdInterfaceName, fromStaticRaw(SystemdEnvironment)});
    auto ret = con.call(envMsg);
    if (ret.type() == QDBusMessage::ErrorMessage) {
        qFatal("%s", ret.errorMessage().toLocal8Bit().data());
    }
    envToPath(qdbus_cast<QStringList>(ret.arguments().constFirst().value<QDBusVariant>().variant()));

    auto sysBus = QDBusConnection::systemBus();
    if (!sysBus.connect(u"org.desktopspec.ApplicationUpdateNotifier1"_s,
                        u"/org/desktopspec/ApplicationUpdateNotifier1"_s,
                        u"org.desktopspec.ApplicationUpdateNotifier1"_s,
                        u"ApplicationUpdated"_s,
                        this,
                        SLOT(ReloadApplications()))) {
        qFatal("connect to ApplicationUpdated failed.");
    }

    auto storagePtr = m_storage.lock();
    if (storagePtr) {
        storagePtr->beginBatchUpdate();
    }

    scanApplications();

    updateAutostartStatus();

    scanInstances();

    scanMimeInfos();

    loadHooks();

    if (storagePtr) {
        if (!storagePtr->setFirstLaunch(false) || !storagePtr->endBatchUpdate()) {
            qCCritical(DDEAM) << "failed to update state of application manager, some properties may be lost after restart.";
        }
    }

    if (auto *ptr = new (std::nothrow)
                     PropertiesForwarder{fromStaticRaw(DDEApplicationManager1ObjectPath),
                                         fromStaticRaw(ApplicationManager1Interface),
                                         this};
        ptr == nullptr) {
        qCCritical(DDEAM) << "new PropertiesForwarder of Application Manager failed.";
    }

    m_startupPhase = false;

    qCInfo(DDEAM) << "Application Manager started.";

    for (const auto &application : std::as_const(m_applicationList)) {
        if (!application->ensurePropertiesForwarder()) {
            qCCritical(DDEAM) << "failed to initialize PropertiesForwarder for" << application->id();
        }
    }

    if (!connection.registerService(fromStaticRaw(DDEApplicationManager1ServiceName))) {
        qFatal("%s", connection.lastError().message().toLocal8Bit().data());
    }

    // TODO: This is a workaround, we will use database at the end.
    const QDir runtimeDir{getXDGRuntimeDir()};
    const auto fileName = runtimeDir.filePath(u"deepin-application-manager"_s);
    QFile flag{fileName};

    auto sessionId = getCurrentSessionId();
    if (flag.open(QFile::ReadOnly | QFile::ExistingOnly)) {
        auto content = flag.read(sessionId.size());
        if (!content.isEmpty() && !sessionId.isEmpty() && content == sessionId) {
            m_isNewSession = false;
            return;
        }
    }

    if (flag.open(QFile::WriteOnly | QFile::Truncate)) {
        flag.write(sessionId, sessionId.size());
        m_isNewSession = true;
        return;
    }

    qCCritical(DDEAM) << "open" << fileName << "failed:" << flag.errorString() << ", AM couldn't specify if it's a new session.";
}

void ApplicationManager1Service::addInstanceToApplication(const QString &unitName,
                                                          const QDBusObjectPath &systemdUnitPath) noexcept
{
    auto info = processUnitName(unitName);
    if (!info) {
        return;
    }

    addInstanceToApplication(std::move(info).value(), systemdUnitPath);
}

void ApplicationManager1Service::addInstanceToApplication(UnitInfo info, const QDBusObjectPath &systemdUnitPath) noexcept
{
    auto appId = std::move(info.applicationID);
    auto launcher = std::move(info.launcher);
    auto instanceId = std::move(info.instanceID);

    if (appId.isEmpty()) {
        return;
    }

    if (instanceId.isEmpty()) {
        instanceId = QUuid::createUuid().toString(QUuid::Id128);
    }

    auto app = m_applicationList.value(appId);

    if (!app) {
        qCWarning(DDEAM) << "couldn't find app" << appId << "in application manager.";
        return;
    }

    app->updateAfterLaunch(sender() != nullptr);  // activate by signal

    const auto &applicationPath = app->applicationPath().path();

    if (!app->addOneInstance(instanceId, applicationPath, systemdUnitPath.path(), launcher)) {
        qCCritical(DDEAM) << "failed to add instance" << systemdUnitPath.path() << "to app" << appId;
    }
}

void ApplicationManager1Service::removeInstanceFromApplication(const QString &unitName,
                                                               const QDBusObjectPath &systemdUnitPath) noexcept
{
    auto info = processUnitName(unitName);
    if (!info) {
        return;
    }

    auto appId = std::move(info->applicationID);

    if (appId.isEmpty()) {
        return;
    }

    auto app = m_applicationList.value(appId);

    if (!app) {
        qWarning() << "couldn't find app" << appId << "in application manager.";
        return;
    }

    const auto &appIns = app->applicationInstances();

    auto instanceIt =
        std::find_if(appIns.cbegin(), appIns.cend(), [&systemdUnitPath](const QSharedPointer<InstanceService> &value) {
            return value->property("SystemdUnitPath") == systemdUnitPath;
        });

    if (instanceIt != appIns.cend()) {
        app->removeOneInstance(instanceIt.key());
        return;
    }

    orphanedInstances.removeIf([&systemdUnitPath](const QSharedPointer<InstanceService> &ptr) {
        return (ptr->property("SystemdUnitPath").value<QDBusObjectPath>() == systemdUnitPath);
    });
}

void ApplicationManager1Service::scanMimeInfos() noexcept
{
    const auto &dirs = getMimeDirs();
    for (const auto &dir : dirs) {
        auto info = MimeInfo::createMimeInfo(dir);
        if (info) {
            m_mimeManager->appendMimeInfo(std::move(info).value());
        }
    }
}

void ApplicationManager1Service::reloadMimeInfos() noexcept
{
    m_mimeManager->reset();
    scanMimeInfos();
    emit m_mimeManager->MimeInfoReloaded();
}

void ApplicationManager1Service::scanApplications() noexcept
{
    forEachApplicationDesktopFile([this](DesktopFile file) -> bool {
        const auto desktopId = file.desktopId();
        if (!addApplication(std::move(file))) {
            qWarning() << "add Application" << desktopId << " failed, skip...";
        }
        return false;
    });
}

void ApplicationManager1Service::scanInstances() noexcept
{
    using namespace Qt::StringLiterals;
    auto &conn = ApplicationManager1DBus::instance().globalDestBus();
    auto call_message = QDBusMessage::createMethodCall(
        SystemdService, SystemdObjectPath, SystemdInterfaceName, fromStaticRaw(SystemdListUnitsByPatterns));
    QList<QVariant> args;
    args << QVariant::fromValue(QStringList{u"running"_s, u"start"_s});
    args << QVariant::fromValue(QStringList{u"dde*"_s, u"deepin*"_s});
    call_message.setArguments(args);
    auto result = conn.call(call_message);
    if (result.type() == QDBusMessage::ErrorMessage) {
        qCritical() << "failed to scan existing instances: call to ListUnits failed:" << result.errorMessage();
        return;
    }

    auto v = result.arguments().constFirst();
    QList<SystemdUnitDBusMessage> units;
    v.value<QDBusArgument>() >> units;
    for (const auto &unit : std::as_const(units)) {
        this->addInstanceToApplication(unit.name, unit.objectPath);
    }
}

void ApplicationManager1Service::updateAutostartStatus() noexcept
{
    forEachAutostartDesktopFile([this](DesktopFile desktopFile) -> bool {
        auto parsedSource = parseAutostartDesktopFile(std::move(desktopFile));
        if (!parsedSource) {
            return false;
        }

        auto desktopId = parsedSource->desktopFile.desktopId();
        auto sourcePath = parsedSource->desktopFile.sourcePath();
        QSharedPointer<ApplicationService> app{nullptr};
        auto asApplication =
            parsedSource->entry.value(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryXDeepinGenerateSource));

        if (asApplication) {  // modified by application manager
            auto appSource = asApplication->get().toString();
            ParserError err{ParserError::NoError};

            const QFileInfo sourceInfo{appSource};
            if (!sourceInfo.exists() || !sourceInfo.isFile()) {
                qWarning() << "autostart file" << appSource << "doesn't exist, skip.";
                return false;
            }

            // add original application
            auto desktopSource = DesktopFile::searchDesktopFileByPath(appSource, err);
            if (err != ParserError::NoError) {
                qWarning() << "search autostart application" << appSource << "failed:" << err;
                return false;
            }
            auto source = std::move(desktopSource).value();

            auto curId = source.desktopId();
            if (curId.isEmpty()) {
                qWarning() << QStringView{DesktopEntryXDeepinGenerateSource} << "is" << appSource
                           << ", but couldn't find it in applications, maybe this autostart file has been modified, skip.";
                return false;
            }

            // add application directly, it wouldn't add the same application twice.
            app = addApplication(std::move(source));
            if (!app) {
                qWarning() << "add autostart application" << curId << "failed, skip.";
                return false;
            }

            app->setAutostartSource({sourcePath, std::move(parsedSource->entry)});
            return false;
        }

        if (auto existApp = m_applicationList.value(desktopId); existApp) {
            existApp->setAutostartSource({sourcePath, std::move(parsedSource->entry)});
            return false;
        }

        if (desktopId.isEmpty() || !parsedSource->desktopFile.hasStandardizedApplicationFileName()) {
            qWarning() << "autostart file" << sourcePath
                       << "doesn't map to an existing application and doesn't have a standard application desktop filename, skip.";
            return false;
        }

        app = addApplication(std::move(parsedSource->desktopFile), std::make_unique<DesktopEntry>(std::move(parsedSource->entry)));
        if (!app) {
            qWarning() << "add autostart application" << desktopId << "failed, skip.";
            return false;
        }

        app->setAutostartSource({sourcePath, {}});
        return false;
    });
}

void ApplicationManager1Service::loadHooks() noexcept
{
    QHash<QString, ApplicationHook> hooks;

    const auto &hooksDirs = getHooksDirs();
    applyIteratively(
        QList<QDir>(hooksDirs.begin(), hooksDirs.end()),
        [&hooks](const QFileInfo &info) -> bool {
            auto fileName = info.fileName();
            if (!hooks.contains(fileName)) {
                if (auto hook = ApplicationHook::loadFromFile(info.absoluteFilePath()); hook) {
                    hooks.insert(fileName, std::move(hook).value());
                }
            }
            return false;
        },
        QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Readable,
        {"*.json"},
        QDir::Name);

    auto hookList = hooks.values();
    std::sort(hookList.begin(), hookList.end());
    m_hookElements = generateHooks(hookList);
}

QList<QDBusObjectPath> ApplicationManager1Service::list() const
{
    QList<QDBusObjectPath> paths;
    for (auto it = m_applicationList.constBegin(); it != m_applicationList.constEnd(); ++it)
        paths << QDBusObjectPath{getObjectPathFromAppId(it.key())};

    return paths;
}

QSharedPointer<ApplicationService> ApplicationManager1Service::addApplication(DesktopFile desktopFileSource) noexcept
{
    return addApplication(std::move(desktopFileSource), nullptr);
}

QSharedPointer<ApplicationService> ApplicationManager1Service::addApplication(DesktopFile desktopFileSource,
                                                                              std::unique_ptr<DesktopEntry> entry) noexcept
{
    if (auto app = m_applicationList.constFind(desktopFileSource.desktopId()); app != m_applicationList.cend()) {
        qInfo() << "this application already exists." << "current desktop source:" << desktopFileSource.sourcePath()
                << "exists app source:" << app->data()->desktopFileSource().sourcePath();
        return *app;
    }

    auto source = desktopFileSource.sourcePath();
    auto application =
        ApplicationService::createApplicationService(std::move(desktopFileSource), std::move(entry), this, m_storage);
    if (!application) {
        qWarning() << "can't create application" << source;
        return nullptr;
    }

    auto *ptr = application.data();
    if (auto *adaptor = new (std::nothrow) ApplicationAdaptor{ptr}; adaptor == nullptr) {
        qCritical() << "new ApplicationAdaptor failed.";
        return nullptr;
    } else {
        setAdaptorAutoRelaySignals(adaptor, false);
    }

    if (!registerObjectToDBus(ptr, application->applicationPath().path(), fromStaticRaw(ApplicationInterface))) {
        return nullptr;
    }
    m_applicationList.insert(application->id(), application);

    if (!m_startupPhase && !application->ensurePropertiesForwarder()) {
        qCCritical(DDEAM) << "failed to initialize PropertiesForwarder for" << application->id();
        return nullptr;
    }

    if (!m_startupPhase) {
        const auto interfaces = getChildInterfacesAndPropertiesFromObject(ptr);
        emit listChanged();
        emit InterfacesAdded(application->applicationPath(), interfaces);
        sendObjectManagerSignal(fromStaticRaw(DDEApplicationManager1ObjectPath),
                                "InterfacesAdded",
                                application->applicationPath(),
                                QVariant::fromValue(interfaces));
    }

    return application;
}

void ApplicationManager1Service::removeOneApplication(const QString &appId) noexcept
{
    auto objectPath = QDBusObjectPath{getObjectPathFromAppId(appId)};
    if (auto it = m_applicationList.constFind(appId); it != m_applicationList.cend()) {
        const auto interfaces = getChildInterfacesFromObject(it->data());
        emit InterfacesRemoved(objectPath, interfaces);
        sendObjectManagerSignal(fromStaticRaw(DDEApplicationManager1ObjectPath),
                                "InterfacesRemoved",
                                objectPath,
                                QVariant::fromValue(interfaces));
        if (auto ptr = m_storage.lock(); ptr) {
            if (!ptr->deleteApplication(appId)) {
                qCritical() << "failed to delete all properties of" << appId;
            }
        }
        unregisterObjectFromDBus(objectPath.path());
        std::ignore = it->data()->RemoveFromDesktop();
        m_applicationList.erase(it);

        emit listChanged();
    }
}

void ApplicationManager1Service::removeAllApplication() noexcept
{
    for (auto it = m_applicationList.constBegin(); it != m_applicationList.constEnd(); ++it) {
        removeOneApplication(it.key());
    }
}

QString ApplicationManager1Service::Identify(const QDBusUnixFileDescriptor &pidfd,
                                             QDBusObjectPath &instance,
                                             ObjectInterfaceMap &application_instance_info) const noexcept
{
    if (!pidfd.isValid()) {
        qWarning() << "pidfd isn't a valid unix file descriptor";
        return {};
    }

    Q_ASSERT_X(static_cast<bool>(m_identifier), "Identify", "Broken Identifier.");

    const auto ret = m_identifier->Identify(pidfd);
    if (ret.ApplicationId.isEmpty()) {
        safe_sendErrorReply(QDBusError::Failed, "Identify failed.");
        return {};
    }

    if (!m_applicationList.contains(ret.ApplicationId)) {
        safe_sendErrorReply(QDBusError::Failed, "can't find application:" % ret.ApplicationId);
        return {};
    }

    auto app = m_applicationList.value(ret.ApplicationId);
    QDBusObjectPath instancePath;
    const auto &instances = app->instances();
    if (ret.InstanceId.isEmpty() && instances.size() == 1) {
        // Maybe a dbus systemd service
        instancePath = instances.constFirst();
    } else {
        instancePath = app->findInstance(ret.InstanceId);
    }
    if (instancePath.path().isEmpty()) {
        safe_sendErrorReply(QDBusError::Failed, "can't find instance:" % ret.InstanceId);
        return {};
    }

    instance = instancePath;
    auto instanceObj = app->m_Instances.constFind(instance);
    application_instance_info = getChildInterfacesAndPropertiesFromObject(instanceObj->get());

    return ret.ApplicationId;
}

void ApplicationManager1Service::updateApplication(const QSharedPointer<ApplicationService> &destApp,
                                                   DesktopFile desktopFile) noexcept
{
    if (!m_applicationList.contains(destApp->id())) {
        return;
    }

    auto *newEntry = new (std::nothrow) DesktopEntry{};
    if (newEntry == nullptr) {
        qCritical() << "new DesktopEntry failed.";
        return;
    }

    auto err = newEntry->parse(desktopFile);
    if (err != ParserError::NoError) {
        qWarning() << "update desktop file failed:" << err << ", content wouldn't change.";
        return;
    }

    if (*(destApp->m_entry) != *newEntry) {
        destApp->resetEntry(newEntry);
        destApp->detachAllInstance();
    }

    if (destApp->m_desktopSource != desktopFile && destApp->isAutoStart()) {
        destApp->m_desktopSource = std::move(desktopFile);
    }
}

void ApplicationManager1Service::ReloadApplications()
{
    if (m_isReloading) {
        qInfo() << "reload already in progress, deferring.";
        m_pendingReload = true;
        return;
    }
    if (calledFromDBus() && !m_reloadTimer.isActive()) {
        doReloadApplications();
        return;
    }
    m_reloadTimer.start();
}

void ApplicationManager1Service::doReloadApplications()
{
    m_isReloading = true;
    m_pendingReload = false;
    qInfo() << "reload applications.";

    auto appIds = m_applicationList.keys();

    forEachApplicationDesktopFile([this, &appIds](DesktopFile file) -> bool {
        auto app = m_applicationList.value(file.desktopId());
        if (app && appIds.contains(app->id())) {
            appIds.removeOne(app->id());
            updateApplication(app, std::move(file));
            return false;
        }

        addApplication(std::move(file));
        return false;
    });

    for (const auto &appId : std::as_const(appIds)) {
        removeOneApplication(appId);
    }

    updateAutostartStatus();

    reloadMimeInfos();
    m_isReloading = false;

    if (m_pendingReload) {
        m_pendingReload = false;
        qInfo() << "pending reload detected, scheduling deferred reload.";
        m_reloadTimer.start();
    }
}

ObjectMap ApplicationManager1Service::GetManagedObjects() const
{
    return dumpDBusObject(m_applicationList);
}

QHash<QDBusObjectPath, QSharedPointer<ApplicationService>>
ApplicationManager1Service::findApplicationsByIds(const QStringList &appIds) const noexcept
{
    QHash<QDBusObjectPath, QSharedPointer<ApplicationService>> ret;
    for (const auto &appId : appIds) {
        if (auto app = m_applicationList.value(appId); app) {
            ret.insert(QDBusObjectPath{getObjectPathFromAppId(appId)}, app);
        }
    }

    return ret;
}

QString ApplicationManager1Service::addUserApplication(const QVariantMap &desktop_file, const QString &name) noexcept
{
    if (name.isEmpty()) {
        safe_sendErrorReply(QDBusError::Failed, "file name is empty.");
        return {};
    }

    QString errMsg;
    auto fileContent = DesktopFileGenerator::generate(desktop_file, errMsg);
    if (fileContent.isEmpty() || !errMsg.isEmpty()) {
        safe_sendErrorReply(QDBusError::Failed, errMsg);
        return {};
    }

    const QDir appDir{getXDGDataHome()};
    const auto &filePath = appDir.filePath(name);
    QFile file{filePath};
    const auto fileExists = file.exists();
    bool shouldRewrite = false;

    if (fileExists) {
        ParserError parseErr{ParserError::NoError};
        // 判断desktop文件内容是否合法
        auto existingDesktopFile = DesktopFile::searchDesktopFileByPath(filePath, parseErr);
        if (existingDesktopFile) {
            // 解析当前已经存在的desktop文件内容
            DesktopEntry entry;
            ParserError entryErr = entry.parse(existingDesktopFile.value());
            if (entryErr != ParserError::NoError) {
                qWarning() << "parse existing desktop file has error:" << entryErr;
                shouldRewrite = true;
            }
        }
    }
    // 根据情况选择打开模式
    QIODevice::OpenMode openMode = QFile::WriteOnly | QFile::Text;
    if (fileExists && shouldRewrite) {
        openMode |= QFile::Truncate;  // 清空现有内容
    } else if (!fileExists) {
        openMode |= QFile::NewOnly;  // 只创建新文件
    }

    if (!file.open(openMode)) {
        safe_sendErrorReply(QDBusError::Failed, file.errorString());
        return {};
    }

    auto writeContent = fileContent.toLocal8Bit();
    if (file.write(writeContent) != writeContent.size()) {
        file.remove();
        safe_sendErrorReply(QDBusError::Failed, "incomplete file content.this file will be removed.");
        return {};
    }

    file.flush();

    ParserError err{ParserError::NoError};
    auto ret = DesktopFile::searchDesktopFileByPath(filePath, err);
    if (err != ParserError::NoError) {
        file.remove();
        qDebug() << "add user's application failed:" << err;
        safe_sendErrorReply(QDBusError::Failed, "search failed.");
        return {};
    }

    if (!ret) {
        file.remove();
        safe_sendErrorReply(QDBusError::InternalError);
        return {};
    }

    auto desktopSource = std::move(ret).value();
    auto appId = desktopSource.desktopId();
    if (!addApplication(std::move(desktopSource))) {
        file.remove();
        safe_sendErrorReply(QDBusError::Failed, "add application to ApplicationManager failed.");
        return {};
    }

    m_mimeManager->updateMimeCache(appDir.absolutePath());
    return appId;
}

void ApplicationManager1Service::deleteUserApplication(const QString &app_id) noexcept
{
    if (app_id.isEmpty()) {
        safe_sendErrorReply(QDBusError::Failed, "app id name is empty.");
        return;
    }

    const QDir appDir{getUserApplicationDir()};
    const auto &filePath = appDir.filePath(app_id % desktopSuffix);

    if (const QFileInfo info{filePath}; !info.exists() || !info.isFile()) {
        safe_sendErrorReply(QDBusError::Failed, QString{"file not exists:%1"}.arg(info.absoluteFilePath()));
        return;
    }

    if (!QFile::remove(filePath)) {
        safe_sendErrorReply(QDBusError::Failed, "remove file failed.");
        return;
    }

    m_mimeManager->updateMimeCache(appDir.absolutePath());
}

QDBusObjectPath ApplicationManager1Service::executeCommand(const QString &program,
                                                           const QStringList &arguments,
                                                           const QString &type,
                                                           const QString &runId,
                                                           const QMap<QString, QString> &envVars,
                                                           const QString &workdir) noexcept
{
    // Validate required parameters
    if (program.isEmpty()) {
        safe_sendErrorReply(QDBusError::Failed, "program path cannot be empty.");
        return QDBusObjectPath("/");
    }

    // Validate execution type
    const QStringList validTypes = {"shortcut", "script", "portablebinary"};
    if (!validTypes.contains(type)) {
        safe_sendErrorReply(QDBusError::Failed,
                            QString{"Invalid type '%1'. Must be one of: shortcut, script, portablebinary"}.arg(type));
        return QDBusObjectPath("/");
    }

    // Check if program exists and is executable
    QFileInfo programInfo(program);
    if (!programInfo.exists()) {
        safe_sendErrorReply(QDBusError::Failed, QString{"Program '%1' does not exist."}.arg(program));
        return QDBusObjectPath("/");
    }
    if (!programInfo.isExecutable()) {
        safe_sendErrorReply(QDBusError::Failed, QString{"Program '%1' is not executable."}.arg(program));
        return QDBusObjectPath("/");
    }

    // Generate runId if not provided (empty string)
    QString actualRunId = runId;
    if (actualRunId.isEmpty()) {
        // Escape the program path to create a valid runId
        actualRunId = program;
    }
    actualRunId = DUtil::escapeToObjectPath(actualRunId);

    // Generate random component for systemd unit name
    QString randomComponent = QUuid::createUuid().toString(QUuid::Id128).mid(1, 8);

    // Construct systemd unit name according to specification
    QString unitName = QString{"app-DDE-tmp.%1.%2@%3.service"}.arg(type, actualRunId, randomComponent);

    // Prepare the command line
    QStringList commandLine;
    commandLine << program;
    commandLine << arguments;

    // Add system environment variables
    QStringList environment;
    QProcessEnvironment systemEnv = QProcessEnvironment::systemEnvironment();
    const auto &keys = systemEnv.keys();
    for (const QString &key : std::as_const(keys)) {
        environment << QString{"%1=%2"}.arg(key, systemEnv.value(key));
    }
    for (auto it = envVars.constBegin(); it != envVars.constEnd(); ++it) {
        environment << QString{"%1=%2"}.arg(it.key(), it.value());
    }

    QList<SystemdProperty> properties;
    // 1. Property: Description
    properties.append({"Description", QDBusVariant(QString("Run: %1").arg(program))});

    // 2. Property: ExecStart (Type: a(sasb))
    // Systemd 要求 ExecStart 是一个结构体数组，因为一个服务可以有多个 ExecStart 命令
    SystemdExecCommand execCmd;
    execCmd.path = program;      // 二进制路径
    execCmd.args = commandLine;  // 完整参数列表 (argv)
    execCmd.unclean = false;     // 是否忽略非零返回值

    QList<SystemdExecCommand> execStartList;
    execStartList << execCmd;
    properties.append({"ExecStart", QDBusVariant(QVariant::fromValue(execStartList))});

    // 3. Property: Environment (Type: as)
    properties.append({"Environment", QDBusVariant(environment)});

    // 4. Property: WorkingDirectory (Type: s)
    if (!workdir.isEmpty()) {
        properties.append({"WorkingDirectory", QDBusVariant(workdir)});
    }

    // Call systemd to start the service
    // StartTransientUnit signature: (s, s, a(sv), a(sa(sv)))
    // args: unit_name, mode("replace"), properties, aux_units

    QDBusConnection conn = QDBusConnection::sessionBus();
    auto msg = QDBusMessage::createMethodCall(QString::fromUtf8(SystemdService),        // Service
                                              QString::fromUtf8(SystemdObjectPath),     // Path
                                              QString::fromUtf8(SystemdInterfaceName),  // Interface
                                              "StartTransientUnit");

    msg.setArguments({
        unitName,                                 // arg1: name
        "replace",                                // arg2: mode
        QVariant::fromValue(properties),          // arg3: properties a(sv)
        QVariant::fromValue(QList<SystemdAux>())  // arg4: aux units (empty)
    });

    auto reply = conn.asyncCall(msg);
    qInfo() << "Request sent to start transient unit (Async):" << unitName;

    // TODO: the return value is currently empty, it's reserved for future use if we plan to make the spawned process as an
    // AM-managed instance At that moment, we should:
    // 1. Add a new API to JobManager1Service to allow add/start a new job without providing an application D-Bus path
    // 2. Use JobManager1Service::addJob to add a new job and run it, like what we do in ApplicationService::Launch()
    // 3. Return the object path that the new JobManager service offered.
    return QDBusObjectPath("/");
}
