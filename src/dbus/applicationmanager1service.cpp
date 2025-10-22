// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "applicationadaptor.h"
#include "applicationchecker.h"
#include "dbus/applicationmanager1adaptor.h"
#include "applicationservice.h"
#include "dbus/AMobjectmanager1adaptor.h"
#include "global.h"
#include "systemdsignaldispatcher.h"
#include "propertiesForwarder.h"
#include "applicationHooks.h"
#include "desktopfilegenerator.h"
#include <QFile>
#include <QHash>
#include <QDBusMessage>
#include <QStringBuilder>
#include <unistd.h>

ApplicationManager1Service::~ApplicationManager1Service() = default;

ApplicationManager1Service::ApplicationManager1Service(std::unique_ptr<Identifier> ptr,
                                                       std::weak_ptr<ApplicationManager1Storage> storage) noexcept
    : m_identifier(std::move(ptr))
    , m_storage(std::move(storage))
{
    using namespace std::chrono_literals;
    m_reloadTimer.setInterval(50);
    m_reloadTimer.setSingleShot(true);
    connect(&m_reloadTimer, &QTimer::timeout, this, &ApplicationManager1Service::doReloadApplications);
}

void ApplicationManager1Service::initService(QDBusConnection &connection) noexcept
{
    if (!connection.registerService(DDEApplicationManager1ServiceName)) {
        qFatal("%s", connection.lastError().message().toLocal8Bit().data());
    }

    if (auto *tmp = new (std::nothrow) ApplicationManager1Adaptor{this}; tmp == nullptr) {
        qCritical() << "new Application Manager Adaptor failed.";
        std::terminate();
    }

    if (auto *tmp = new (std::nothrow) AMObjectManagerAdaptor{this}; tmp == nullptr) {
        qCritical() << "new Object Manager of Application Manager Adaptor failed.";
        std::terminate();
    }

    if (!registerObjectToDBus(this, DDEApplicationManager1ObjectPath, ApplicationManager1Interface)) {
        std::terminate();
    }

    if (m_jobManager.reset(new (std::nothrow) JobManager1Service(this)); !m_jobManager) {
        qCritical() << "new JobManager failed.";
        std::terminate();
    }

    if (m_mimeManager.reset(new (std::nothrow) MimeManager1Service(this)); !m_mimeManager) {
        qCritical() << "new MimeManager failed.";
        std::terminate();
    }

    if (m_compatibilityManager.reset(new (std::nothrow) CompatibilityManager()); !m_compatibilityManager) {
        qWarning() << "new CompatibilityManager failed.";
    }

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &ApplicationManager1Service::ReloadApplications);
    auto unhandled = m_watcher.addPaths(getDesktopFileDirs());
    for (const auto &dir : unhandled) {
        qCritical() << "couldn't watch directory:" << dir;
    }

    auto &dispatcher = SystemdSignalDispatcher::instance();

    connect(&dispatcher, &SystemdSignalDispatcher::SystemdUnitNew, this, &ApplicationManager1Service::addInstanceToApplication);
    connect(
        &dispatcher, &SystemdSignalDispatcher::SystemdJobNew, this, [this](QString unitName, QDBusObjectPath systemdUnitPath) {
            auto info = processUnitName(unitName);
            auto appId = std::move(info.applicationID);

            if (appId.isEmpty()) {
                return;
            }

            auto app = m_applicationList.value(appId);

            if (!app) {
                return;
            }

            // 服务在 AM 之后启动那么 instance size 是 0， newJob 时尝试添加一次
            // 比如 dde-file-manager.service 如果启动的比 AM 晚，那么在 scanInstances 时不会 addInstanceToApplication
            if (app->instances().size() > 0) {
                return;
            }

            qDebug() << "add Instance " << unitName << "on JobNew, " << app->instances().size();

            addInstanceToApplication(unitName, systemdUnitPath);
        });

    connect(&dispatcher,
            &SystemdSignalDispatcher::SystemdUnitRemoved,
            this,
            &ApplicationManager1Service::removeInstanceFromApplication);

    auto envToPath = [this](const QStringList &envs) {
        if (auto path = std::find_if(envs.cbegin(), envs.cend(), [](const QString &env) { return env.startsWith("PATH="); });
            path != envs.cend()) {
            m_systemdPathEnv = path->mid(5).split(':', Qt::SkipEmptyParts);
        }
    };

    connect(&dispatcher, &SystemdSignalDispatcher::SystemdEnvironmentChanged, envToPath);

    auto &con = ApplicationManager1DBus::instance().globalDestBus();
    auto envMsg = QDBusMessage::createMethodCall(SystemdService, SystemdObjectPath, SystemdPropInterfaceName, "Get");
    envMsg.setArguments({SystemdInterfaceName, "Environment"});
    auto ret = con.call(envMsg);
    if (ret.type() == QDBusMessage::ErrorMessage) {
        qFatal("%s", ret.errorMessage().toLocal8Bit().data());
    }
    envToPath(qdbus_cast<QStringList>(ret.arguments().first().value<QDBusVariant>().variant()));

    auto sysBus = QDBusConnection::systemBus();
    if (!sysBus.connect("org.desktopspec.ApplicationUpdateNotifier1",
                        "/org/desktopspec/ApplicationUpdateNotifier1",
                        "org.desktopspec.ApplicationUpdateNotifier1",
                        "ApplicationUpdated",
                        this,
                        SLOT(ReloadApplications()))) {
        qFatal("connect to ApplicationUpdated failed.");
    }

    scanMimeInfos();

    scanApplications();

    auto needLaunch = scanAutoStart();

    scanInstances();

    auto storagePtr = m_storage.lock();
    if (!storagePtr->setFirstLaunch(false)) {
        qCritical() << "failed to update state of application manager, some properties may be lost after restart.";
    }

    loadHooks();

    if (auto *ptr = new (std::nothrow) PropertiesForwarder{DDEApplicationManager1ObjectPath, this}; ptr == nullptr) {
        qCritical() << "new PropertiesForwarder of Application Manager failed.";
    }

    // TODO: This is a workaround, we will use database at the end.
    auto runtimePath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimePath.isEmpty()) {
        runtimePath = QString{"/run/user/%1/"}.arg(getCurrentUID());
    }

    QDir runtimeDir{runtimePath};
    const auto *filename = u8"deepin-application-manager";
    QFile flag{runtimeDir.filePath(filename)};

    auto sessionId = getCurrentSessionId();
    if (flag.open(QFile::ReadOnly | QFile::ExistingOnly)) {
        auto content = flag.read(sessionId.size());
        if (!content.isEmpty() and !sessionId.isEmpty() and content == sessionId) {
            return;
        }
        flag.close();
    }

    if (flag.open(QFile::WriteOnly | QFile::Truncate)) {
        flag.write(sessionId, sessionId.size());
    }
    auto value = QString::fromLocal8Bit(qgetenv("XDG_SESSION_TYPE"));
    if (!value.isEmpty() && value == QString::fromLocal8Bit("wayland")) {
        for (const auto &[app, realExec] : needLaunch.asKeyValueRange()) {
            app->Launch({}, {}, {}, realExec);
        }
    } else {
        constexpr auto XSettings = "org.deepin.dde.XSettings1";

        auto *watcher = new (std::nothrow)
            QDBusServiceWatcher{XSettings, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration, this};

        auto *sigCon = new (std::nothrow) QMetaObject::Connection{};

        auto singleSlot = [watcher, sigCon, autostartMap = std::move(needLaunch)]() {
            QObject::disconnect(*sigCon);
            delete sigCon;
            qDebug() << XSettings << "is registered.";

            for (const auto &[app, realExec] : autostartMap.asKeyValueRange()) {
                app->Launch({}, {}, {}, realExec);
            }

            watcher->deleteLater();
        };

        if (watcher != nullptr) {
            *sigCon = connect(watcher, &QDBusServiceWatcher::serviceRegistered, singleSlot);
        }

        auto msg = QDBusMessage::createMethodCall(
            "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameHasOwner");
        msg << XSettings;

        auto reply = QDBusConnection::sessionBus().call(msg);
        if (reply.type() != QDBusMessage::ReplyMessage) {
            qWarning() << "call org.freedesktop.DBus::NameHasOwner failed, skip autostart:" << reply.errorMessage();
            // The connection should not be deleted, failure to call org.freedesktop.DBus::NameHasOwner does not mean that the
            // XSettings service is invalid.
            return;
        }

        if (reply.arguments().first().toBool()) {
            singleSlot();
        }
    }
}

void ApplicationManager1Service::addInstanceToApplication(const QString &unitName, const QDBusObjectPath &systemdUnitPath)
{
    auto info = processUnitName(unitName);
    auto appId = std::move(info.applicationID);
    auto launcher = std::move(info.Launcher);
    auto instanceId = std::move(info.instanceID);

    if (appId.isEmpty()) {
        return;
    }
    if (instanceId.isEmpty()) {
        instanceId = QUuid::createUuid().toString(QUuid::Id128);
    }

    auto app = m_applicationList.value(appId);

    if (!app) {
        qWarning() << "couldn't find app" << appId << "in application manager.";
        return;
    }

    app->updateAfterLaunch(sender() != nullptr);  // activate by signal

    const auto &applicationPath = app->applicationPath().path();

    if (!app->addOneInstance(instanceId, applicationPath, systemdUnitPath.path(), launcher)) {
        qCritical() << "add Instance failed:" << applicationPath << unitName << systemdUnitPath.path();
    }
}

void ApplicationManager1Service::removeInstanceFromApplication(const QString &unitName, const QDBusObjectPath &systemdUnitPath)
{
    auto info = processUnitName(unitName);
    auto appId = std::move(info.applicationID);
    auto launcher = std::move(info.Launcher);
    auto instanceId = std::move(info.instanceID);

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
    const auto& dirs = getMimeDirs();
    for (const auto &dir : dirs) {
        auto info = MimeInfo::createMimeInfo(dir);
        if (info) {
            m_mimeManager->appendMimeInfo(std::move(info).value());
        }
    }
}

void ApplicationManager1Service::scanApplications() noexcept
{
    const auto &desktopFileDirs = getDesktopFileDirs();

    std::map<QString, DesktopFile> fileMap;
    applyIteratively(
        QList<QDir>(desktopFileDirs.crbegin(), desktopFileDirs.crend()),
        [&fileMap](const QFileInfo &info) -> bool {
            ParserError err{ParserError::NoError};
            auto ret = DesktopFile::searchDesktopFileByPath(info.absoluteFilePath(), err);
            if (!ret.has_value()) {
                qWarning() << "failed to search File:" << err;
                return false;
            }
            auto file = std::move(ret).value();
            fileMap.insert_or_assign(file.desktopId(), std::move(file));
            return false;  // means to apply this function to the rest of the files
        },
        QDir::Readable | QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
        {"*.desktop"},
        QDir::Name | QDir::DirsLast);

    for (auto &&[k, v] : std::move(fileMap)) {
        if (!addApplication(std::move(v))) {
            qWarning() << "add Application" << k << " failed, skip...";
        }
    }
}

void ApplicationManager1Service::scanInstances() noexcept
{
    auto &conn = ApplicationManager1DBus::instance().globalDestBus();
    auto call_message =
        QDBusMessage::createMethodCall(SystemdService, SystemdObjectPath, SystemdInterfaceName, "ListUnitsByPatterns");
    QList<QVariant> args;
    args << QVariant::fromValue(QStringList{"running", "start"});
    args << QVariant::fromValue(QStringList{"dde*", "deepin*"});
    call_message.setArguments(args);
    auto result = conn.call(call_message);
    if (result.type() == QDBusMessage::ErrorMessage) {
        qCritical() << "failed to scan existing instances: call to ListUnits failed:" << result.errorMessage();
        return;
    }

    auto v = result.arguments().first();
    QList<SystemdUnitDBusMessage> units;
    v.value<QDBusArgument>() >> units;
    for (const auto &unit : units) {
        this->addInstanceToApplication(unit.name, unit.objectPath);
    }
}

QHash<QSharedPointer<ApplicationService>, QString> ApplicationManager1Service::scanAutoStart() noexcept
{
    QHash<QSharedPointer<ApplicationService>, QString> ret;
    auto autostartDirs = getAutoStartDirs();
    std::map<QString, DesktopFile> autostartItems;

    applyIteratively(
        QList<QDir>{autostartDirs.crbegin(), autostartDirs.crend()},
        [&autostartItems](const QFileInfo &info) {
            ParserError err{ParserError::InternalError};
            auto desktopSource = DesktopFile::searchDesktopFileByPath(info.absoluteFilePath(), err);
            if (err != ParserError::NoError) {
                qWarning() << "skip" << info.absoluteFilePath();
                return false;
            }
            auto file = std::move(desktopSource).value();
            autostartItems.insert_or_assign(file.desktopId(), std::move(file));
            return false;
        },
        QDir::Readable | QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
        {"*.desktop"},
        QDir::Name | QDir::DirsLast);

    for (auto &it : autostartItems) {
        auto desktopFile = std::move(it.second);
        DesktopFileGuard guard{desktopFile};
        if (!guard.try_open()) {
            continue;
        }

        auto &file = desktopFile.sourceFileRef();
        QTextStream stream{&file};
        DesktopEntry tmp;
        auto err = tmp.parse(stream);
        if (err != ParserError::NoError) {
            qWarning() << "parse autostart file" << desktopFile.sourcePath() << " error:" << err;
            continue;
        }

        QSharedPointer<ApplicationService> app{nullptr};
        auto asApplication = tmp.value(DesktopFileEntryKey, X_Deepin_GenerateSource).value_or(DesktopEntry::Value{});
        auto shouldLaunch = tmp.value(DesktopFileEntryKey, DesktopEntryHidden).value_or(DesktopEntry::Value{});
        auto autostartFlag = shouldLaunch.isNull() || (shouldLaunch.toString().compare("true", Qt::CaseInsensitive) != 0);

        if (!asApplication.isNull()) {  // modified by application manager
            auto appSource = asApplication.toString();

            const QFileInfo sourceInfo{appSource};
            if (!sourceInfo.exists() || !sourceInfo.isFile()) {
                qWarning() << "autostart file" << appSource << "doesn't exist, skip.";
                continue;
            }

            // add original application
            auto desktopSource = DesktopFile::searchDesktopFileByPath(appSource, err);
            if (err != ParserError::NoError) {
                qWarning() << "search autostart application failed:" << err;
                continue;
            }
            auto source = std::move(desktopSource).value();

            if (source.desktopId().isEmpty()) {
                qWarning() << X_Deepin_GenerateSource << "is" << appSource
                           << ", but couldn't find it in applications, maybe this autostart file has been modified, skip.";
                continue;
            }

            // add application directly, it wouldn't add the same application twice.
            app = addApplication(std::move(source));
            if (!app) {
                qWarning() << "add autostart application failed, skip.";
                continue;
            }

            app->setAutostartSource({desktopFile.sourcePath(), std::move(tmp)});
            if (autostartFlag) {
                auto val = tmp.value(DesktopFileEntryKey, "Exec").value_or(DesktopEntry::Value{});
                auto realExec = val.isNull() ? QString{} : val.toString();
                qInfo() << "launch autostart application " << app->id() << " by " << realExec;
                ret.insert(app, realExec);
            }

            continue;
        }

        // maybe some application generate autostart desktop by itself
        if (ApplicationFilter::tryExecCheck(tmp) or ApplicationFilter::showInCheck(tmp)) {
            qInfo() << "autostart application couldn't pass check.";
            continue;
        }

        if (auto existApp = m_applicationList.value(desktopFile.desktopId()); existApp) {
            if (autostartFlag) {
                auto val = tmp.value(DesktopFileEntryKey, "Exec").value_or(DesktopEntry::Value{});
                auto realExec = val.isNull() ? QString{} : val.toString();
                qInfo() << "launch exist autostart application " << existApp->id() << " by " << realExec;
                ret.insert(existApp, realExec);
            }

            existApp->setAutostartSource({desktopFile.sourcePath(), std::move(tmp)});
            continue;
        }

        guard.close();
        // new application
        auto originalSource = desktopFile.sourcePath();
        app = addApplication(std::move(desktopFile));
        if (!app) {
            qWarning() << "add autostart application failed, skip.";
            continue;
        }
        app->setAutostartSource({originalSource, {}});

        if (autostartFlag) {
            qInfo() << "launch new autostart application " << app->id();
            ret.insert(app, {});
        }
    }

    return ret;
}

void ApplicationManager1Service::loadHooks() noexcept
{
    auto hookDirs = getXDGDataMergedDirs();
    std::for_each(hookDirs.begin(), hookDirs.end(), [](QString &str) { str.append(ApplicationManagerHookDir); });
    QHash<QString, ApplicationHook> hooks;

    applyIteratively(
        QList<QDir>(hookDirs.begin(), hookDirs.end()),
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
    for (const auto &appId : m_applicationList.keys())
        paths << QDBusObjectPath{getObjectPathFromAppId(appId)};

    return paths;
}

QSharedPointer<ApplicationService> ApplicationManager1Service::addApplication(DesktopFile desktopFileSource) noexcept
{
    if (auto app = m_applicationList.constFind(desktopFileSource.desktopId()); app != m_applicationList.cend()) {
        qInfo() << "this application already exists." << "current desktop source:" << desktopFileSource.sourcePath()
                << "exists app source:" << app->data()->desktopFileSource().sourcePath();
        return *app;
    }

    auto source = desktopFileSource.sourcePath();
    QSharedPointer<ApplicationService> application =
        ApplicationService::createApplicationService(std::move(desktopFileSource), this, m_storage);
    if (!application) {
        qWarning() << "can't create application" << source;
        return nullptr;
    }

    auto *ptr = application.data();
    if (auto *adaptor = new (std::nothrow) ApplicationAdaptor{ptr}; adaptor == nullptr) {
        qCritical() << "new ApplicationAdaptor failed.";
        return nullptr;
    }

    if (!registerObjectToDBus(ptr, application->applicationPath().path(), ApplicationInterface)) {
        return nullptr;
    }
    m_applicationList.insert(application->id(), application);

    emit listChanged();
    emit InterfacesAdded(application->applicationPath(), getChildInterfacesAndPropertiesFromObject(ptr));

    return application;
}

void ApplicationManager1Service::removeOneApplication(const QString &appId) noexcept
{
    auto objectPath = QDBusObjectPath{getObjectPathFromAppId(appId)};
    if (auto it = m_applicationList.find(appId); it != m_applicationList.cend()) {
        emit InterfacesRemoved(objectPath, getChildInterfacesFromObject(it->data()));
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
    for (const auto &appId : m_applicationList.keys()) {
        removeOneApplication(appId);
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

    if (destApp->m_desktopSource != desktopFile and destApp->isAutoStart()) {
        destApp->m_desktopSource = std::move(desktopFile);
    }
}

void ApplicationManager1Service::ReloadApplications()
{
    if (calledFromDBus() && !m_reloadTimer.isActive()) {
        doReloadApplications();
        return;
    }
    m_reloadTimer.start();
}

void ApplicationManager1Service::doReloadApplications()
{
    qInfo() << "reload applications.";

    auto desktopFileDirs = getDesktopFileDirs();
    desktopFileDirs.append(getXDGConfigDirs());
    auto appIds = m_applicationList.keys();

    applyIteratively(
        QList<QDir>(desktopFileDirs.cbegin(), desktopFileDirs.cend()),
        [this, &appIds](const QFileInfo &info) -> bool {
            ParserError err{ParserError::NoError};
            auto ret = DesktopFile::searchDesktopFileByPath(info.absoluteFilePath(), err);
            if (!ret.has_value()) {
                return false;
            }

            auto file = std::move(ret).value();
            auto app = m_applicationList.value(file.desktopId());
            if (app && appIds.contains(app->id())) {
                // Can emit correct remove signal when uninstalling applications
                if (ApplicationFilter::tryExecCheck(*(app.data()->m_entry))) {
                    qDebug() << info.absolutePath() << "Checked TryExec failed and will be removed";
                    return false;
                }
                appIds.removeOne(app->id());
                updateApplication(app, std::move(file));
                return false;
            }

            addApplication(std::move(file));
            return false;
        },
        QDir::Readable | QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
        {"*.desktop"},
        QDir::Name | QDir::DirsLast);

    for (const auto &appId : appIds) {
        removeOneApplication(appId);
    }

    m_mimeManager->reset();
    scanMimeInfos();
    scanAutoStart();
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

    // 传入的desktop文件内容为空的时候直接返回。
    QString errMsg;
    auto fileContent = DesktopFileGenerator::generate(desktop_file, errMsg);
    if (fileContent.isEmpty() or !errMsg.isEmpty()) {
        safe_sendErrorReply(QDBusError::Failed, errMsg);
        return {};
    }

    QDir xdgDataHome;
    QString dir{getXDGDataHome() + "/applications"};
    if (!xdgDataHome.mkpath(dir)) {
        safe_sendErrorReply(QDBusError::Failed, "couldn't create directory of user applications.");
        return {};
    }

    // 判断当前是否已经存在了desktop文件
    xdgDataHome.setPath(dir);
    const auto &filePath = xdgDataHome.filePath(name);
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

    m_mimeManager->updateMimeCache(dir);
    return appId;
}

void ApplicationManager1Service::deleteUserApplication(const QString &app_id) noexcept
{
    if (app_id.isEmpty()) {
        safe_sendErrorReply(QDBusError::Failed, "app id name is empty.");
        return;
    }

    QDir xdgDataHome;
    QString dir{getXDGDataHome() + "/applications"};
    if (!xdgDataHome.mkpath(dir)) {
        safe_sendErrorReply(QDBusError::Failed, "couldn't create directory of user applications.");
        return;
    }

    xdgDataHome.setPath(dir);
    const auto &filePath = xdgDataHome.filePath(app_id + ".desktop");

    if (QFileInfo info{filePath}; !info.exists() || !info.isFile()) {
        safe_sendErrorReply(QDBusError::Failed, QString{"file not exists:%1"}.arg(info.absoluteFilePath()));
        return;
    }

    /*
        auto apps = findApplicationsByIds({app_id});
        if (apps.isEmpty()) {
            qWarning() << "we can't find corresponding application in ApplicationManagerService.";
        }

        for (const auto &app : apps) {
            app->setMimeTypes({});
        }
    */
    if (!QFile::remove(filePath)) {
        safe_sendErrorReply(QDBusError::Failed, "remove file failed.");
        return;
    }

    m_mimeManager->updateMimeCache(dir);
}
