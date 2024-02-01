// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "applicationadaptor.h"
#include "applicationchecker.h"
#include "dbus/applicationmanager1adaptor.h"
#include "applicationservice.h"
#include "dbus/AMobjectmanager1adaptor.h"
#include "systemdsignaldispatcher.h"
#include "propertiesForwarder.h"
#include "applicationHooks.h"
#include "desktopfilegenerator.h"
#include "processguesser1service.h"
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
}

void ApplicationManager1Service::initService(QDBusConnection &connection) noexcept
{
    if (!connection.registerService(DDEApplicationManager1ServiceName)) {
        qFatal("%s", connection.lastError().message().toLocal8Bit().data());
    }

    new (std::nothrow) ProcessGuesser1Service{connection, this};

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

    auto &dispatcher = SystemdSignalDispatcher::instance();

    connect(&dispatcher, &SystemdSignalDispatcher::SystemdUnitNew, this, &ApplicationManager1Service::addInstanceToApplication);

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
    storagePtr->setFirstLaunch(false);

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

    *sigCon = connect(watcher, &QDBusServiceWatcher::serviceRegistered, singleSlot);

    auto msg =
        QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameHasOwner");
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

void ApplicationManager1Service::addInstanceToApplication(const QString &unitName, const QDBusObjectPath &systemdUnitPath)
{
    if (!isApplication(systemdUnitPath)) {
        return;
    }

    auto info = processUnitName(unitName);
    auto appId = std::move(info.applicationID);
    auto launcher = std::move(info.Launcher);
    auto instanceId = std::move(info.instanceID);

    if (appId.isEmpty()) {
        return;
    }

    auto appIt = std::find_if(m_applicationList.cbegin(),
                              m_applicationList.cend(),
                              [&appId](const QSharedPointer<ApplicationService> &app) { return app->id() == appId; });

    if (appIt == m_applicationList.cend()) [[unlikely]] {
        qWarning() << "couldn't find app" << appId << "in application manager.";
        return;
    }

    (*appIt)->updateAfterLaunch(sender() != nullptr);  // activate by signal

    const auto &applicationPath = (*appIt)->applicationPath().path();

    if (!(*appIt)->addOneInstance(instanceId, applicationPath, systemdUnitPath.path(), launcher)) [[likely]] {
        qCritical() << "add Instance failed:" << applicationPath << unitName << systemdUnitPath.path();
    }
}

void ApplicationManager1Service::removeInstanceFromApplication(const QString &unitName, const QDBusObjectPath &systemdUnitPath)
{
    if (!isApplication(systemdUnitPath)) {
        return;
    }

    auto info = processUnitName(unitName);
    auto appId = std::move(info.applicationID);
    auto launcher = std::move(info.Launcher);
    auto instanceId = std::move(info.instanceID);

    if (appId.isEmpty()) {
        return;
    }

    auto appIt = std::find_if(m_applicationList.cbegin(),
                              m_applicationList.cend(),
                              [&appId](const QSharedPointer<ApplicationService> &app) { return app->id() == appId; });

    if (appIt == m_applicationList.cend()) [[unlikely]] {
        qWarning() << "couldn't find app" << appId << "in application manager.";
        return;
    }

    const auto &appIns = (*appIt)->applicationInstances();

    auto instanceIt =
        std::find_if(appIns.cbegin(), appIns.cend(), [&systemdUnitPath](const QSharedPointer<InstanceService> &value) {
            return value->property("SystemdUnitPath") == systemdUnitPath;
        });

    if (instanceIt != appIns.cend()) [[likely]] {
        (*appIt)->removeOneInstance(instanceIt.key());
        return;
    }

    orphanedInstances->removeIf([&systemdUnitPath](const QSharedPointer<InstanceService> &ptr) {
        return (ptr->property("SystemdUnitPath").value<QDBusObjectPath>() == systemdUnitPath);
    });
}

void ApplicationManager1Service::scanMimeInfos() noexcept
{
    QStringList dirs;
    dirs.append(getXDGConfigDirs());
    dirs.append(getDesktopFileDirs());

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
    auto call_message = QDBusMessage::createMethodCall(SystemdService, SystemdObjectPath, SystemdInterfaceName, "ListUnits");
    auto result = conn.call(call_message);
    if (result.type() == QDBusMessage::ErrorMessage) {
        qCritical() << "failed to scan existing instances: call to ListUnits failed:" << result.errorMessage();
        return;
    }

    auto v = result.arguments().first();
    QList<SystemdUnitDBusMessage> units;
    v.value<QDBusArgument>() >> units;
    for (const auto &unit : units) {
        if (!isApplication(unit.objectPath)) {
            continue;
        }
        if (unit.subState == "running") {
            this->addInstanceToApplication(unit.name, unit.objectPath);
        }
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
        if (!asApplication.isNull()) {  // modified by application manager
            auto appSource = asApplication.toString();

            QFileInfo sourceInfo{appSource};
            if ((!sourceInfo.exists() or !sourceInfo.isFile()) and !file.remove()) {
                qWarning() << "remove invalid autostart file error:" << file.error();
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
        }

        auto shouldLaunch = tmp.value(DesktopFileEntryKey, DesktopEntryHidden).value_or(DesktopEntry::Value{});
        if (!shouldLaunch.isNull() and (shouldLaunch.toString().compare("true", Qt::CaseInsensitive) == 0)) {
            qInfo() << "shouldn't launch this autoStart item.";
            continue;
        }

        if (app) {
            auto realExec = tmp.value(DesktopFileEntryKey, "Exec").value_or(QString{""}).toString();
            qInfo() << "launch normal autostart application " << app->id() << " by " << realExec;
            ret.insert(app, realExec);
            app->setAutostartSource({desktopFile.sourcePath(), std::move(tmp)});
            continue;
        }

        // maybe some application generate autostart desktop by itself
        if (ApplicationFilter::tryExecCheck(tmp) or ApplicationFilter::showInCheck(tmp)) {
            qInfo() << "autostart application couldn't pass check.";
            continue;
        }

        if (auto appIt = std::find_if(m_applicationList.cbegin(),
                                      m_applicationList.cend(),
                                      [&desktopFile](const auto &app) { return desktopFile.desktopId() == app->id(); });
            appIt != m_applicationList.cend()) {
            auto realExec = tmp.value(DesktopFileEntryKey, "Exec").value_or(QString{""}).toString();
            qInfo() << "launch exist autostart application " << (*appIt)->id() << " by " << realExec;
            ret.insert(*appIt, realExec);
            (*appIt)->setAutostartSource({desktopFile.sourcePath(), std::move(tmp)});
            continue;
        }

        guard.close();
        // new application
        auto newApp = addApplication(std::move(desktopFile));
        if (!newApp) {
            qWarning() << "add autostart application failed, skip.";
            continue;
        }

        qInfo() << "launch new autostart application " << newApp->id();
        newApp->setAutostartSource({desktopFile.sourcePath()});
        ret.insert(newApp, {});
    }

    return ret;
}

void ApplicationManager1Service::loadHooks() noexcept
{
    auto hookDirs = getXDGDataDirs();
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
    return m_applicationList.keys();
}

QSharedPointer<ApplicationService> ApplicationManager1Service::addApplication(DesktopFile desktopFileSource) noexcept
{
    auto objectPath = QDBusObjectPath{getObjectPathFromAppId(desktopFileSource.desktopId())};
    if (auto app = m_applicationList.constFind(objectPath); app != m_applicationList.cend()) {
        qInfo() << "this application already exists."
                << "current desktop source:" << desktopFileSource.sourcePath()
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
    m_applicationList.insert(application->applicationPath(), application);

    emit listChanged();
    emit InterfacesAdded(application->applicationPath(), getChildInterfacesAndPropertiesFromObject(ptr));

    return application;
}

void ApplicationManager1Service::removeOneApplication(const QDBusObjectPath &application) noexcept
{
    if (auto it = m_applicationList.find(application); it != m_applicationList.cend()) {
        emit InterfacesRemoved(application, getChildInterfacesFromObject(it->data()));
        if (auto ptr = m_storage.lock(); ptr) {
            ptr->deleteApplication((*it)->id());
        }
        unregisterObjectFromDBus(application.path());
        m_applicationList.remove(application);

        emit listChanged();
    }
}

void ApplicationManager1Service::removeAllApplication() noexcept
{
    for (const auto &app : m_applicationList.keys()) {
        removeOneApplication(app);
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

    auto pid = getPidFromPidFd(pidfd);
    if (pid == 0) {
        sendErrorReply(QDBusError::Failed, "pid is invalid");
        return {};
    }

    const auto ret = m_identifier->Identify(pid);
    if (ret.ApplicationId.isEmpty()) {
        sendErrorReply(QDBusError::Failed, "Identify failed.");
        return {};
    }

    auto app = std::find_if(m_applicationList.cbegin(), m_applicationList.cend(), [&ret](const auto &appPtr) {
        return appPtr->id() == ret.ApplicationId;
    });

    if (app == m_applicationList.cend()) {
        sendErrorReply(QDBusError::Failed, "can't find application:" % ret.ApplicationId);
        return {};
    }

    auto instancePath = (*app)->findInstance(ret.InstanceId);

    if (auto path = instancePath.path(); path.isEmpty()) {
        sendErrorReply(QDBusError::Failed, "can't find instance:" % path);
        return {};
    }

    instance = instancePath;
    auto instanceObj = (*app)->m_Instances.constFind(instance);
    application_instance_info = getChildInterfacesAndPropertiesFromObject(instanceObj->get());

    return ret.ApplicationId;
}

void ApplicationManager1Service::updateApplication(const QSharedPointer<ApplicationService> &destApp,
                                                   DesktopFile desktopFile) noexcept
{
    if (auto app = m_applicationList.find(destApp->applicationPath()); app == m_applicationList.cend()) {
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

    if (destApp->m_entry != newEntry) {
        destApp->resetEntry(newEntry);
        destApp->detachAllInstance();
    }

    if (destApp->m_desktopSource != desktopFile and destApp->isAutoStart()) {
        destApp->setAutoStart(false);
        destApp->m_desktopSource = std::move(desktopFile);
        destApp->setAutoStart(true);
    }
}

void ApplicationManager1Service::ReloadApplications()
{
    const auto &desktopFileDirs = getDesktopFileDirs();

    auto apps = m_applicationList.keys();

    applyIteratively(
        QList<QDir>(desktopFileDirs.cbegin(), desktopFileDirs.cend()),
        [this, &apps](const QFileInfo &info) -> bool {
            ParserError err{ParserError::NoError};
            auto ret = DesktopFile::searchDesktopFileByPath(info.absoluteFilePath(), err);
            if (!ret.has_value()) {
                return false;
            }

            auto file = std::move(ret).value();

            auto destApp =
                std::find_if(m_applicationList.cbegin(),
                             m_applicationList.cend(),
                             [&file](const QSharedPointer<ApplicationService> &app) { return file.desktopId() == app->id(); });

            if (err != ParserError::NoError) {
                qWarning() << "error occurred:" << err << " skip this application.";
                return false;
            }

            if (destApp != m_applicationList.cend() and apps.contains(destApp.key())) {
                apps.removeOne(destApp.key());
                updateApplication(destApp.value(), std::move(file));
                return false;
            }

            addApplication(std::move(file));
            return false;
        },
        QDir::Readable | QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
        {"*.desktop"},
        QDir::Name | QDir::DirsLast);

    for (const auto &key : apps) {
        removeOneApplication(key);
    }

    m_mimeManager->reset();
    scanMimeInfos();

    scanAutoStart();
}

ObjectMap ApplicationManager1Service::GetManagedObjects() const
{
    return dumpDBusObject(m_applicationList);
}

QMap<QDBusObjectPath, QSharedPointer<ApplicationService>>
ApplicationManager1Service::findApplicationsByIds(const QStringList &appIds) const noexcept
{
    QMap<QDBusObjectPath, QSharedPointer<ApplicationService>> ret;
    for (auto it = m_applicationList.constKeyValueBegin(); it != m_applicationList.constKeyValueEnd(); ++it) {
        const auto &ptr = it->second;
        if (appIds.contains(ptr->id())) {
            ret.insert(it->first, it->second);
        }
    }

    return ret;
}

QString ApplicationManager1Service::addUserApplication(const QVariantMap &desktop_file, const QString &name) noexcept
{
    if (name.isEmpty()) {
        sendErrorReply(QDBusError::Failed, "file name is empty.");
        return {};
    }

    QDir xdgDataHome;
    QString dir{getXDGDataHome() + "/applications"};
    if (!xdgDataHome.mkpath(dir)) {
        sendErrorReply(QDBusError::Failed, "couldn't create directory of user applications.");
        return {};
    }

    xdgDataHome.setPath(dir);
    const auto &filePath = xdgDataHome.filePath(name);

    if (QFileInfo info{filePath}; info.exists() and info.isFile()) {
        sendErrorReply(QDBusError::Failed, QString{"file already exists:%1"}.arg(info.absoluteFilePath()));
        return {};
    }

    QFile file{filePath};
    if (!file.open(QFile::NewOnly | QFile::WriteOnly | QFile::Text)) {
        sendErrorReply(QDBusError::Failed, file.errorString());
        return {};
    }

    QString errMsg;
    auto fileContent = DesktopFileGenerator::generate(desktop_file, errMsg);
    if (fileContent.isEmpty() or !errMsg.isEmpty()) {
        file.remove();
        sendErrorReply(QDBusError::Failed, errMsg);
        return {};
    }

    auto writeContent = fileContent.toLocal8Bit();
    if (file.write(writeContent) != writeContent.size()) {
        file.remove();
        sendErrorReply(QDBusError::Failed, "incomplete file content.this file will be removed.");
        return {};
    }

    file.flush();

    ParserError err{ParserError::NoError};
    auto ret = DesktopFile::searchDesktopFileByPath(filePath, err);
    if (err != ParserError::NoError) {
        file.remove();
        qDebug() << "add user's application failed:" << err;
        sendErrorReply(QDBusError::Failed, "search failed.");
        return {};
    }

    if (!ret) {
        file.remove();
        sendErrorReply(QDBusError::InternalError);
        return {};
    }

    auto desktopSource = std::move(ret).value();
    auto appId = desktopSource.desktopId();
    if (!addApplication(std::move(desktopSource))) {
        file.remove();
        sendErrorReply(QDBusError::Failed, "add application to ApplicationManager failed.");
        return {};
    }

    return appId;
}
