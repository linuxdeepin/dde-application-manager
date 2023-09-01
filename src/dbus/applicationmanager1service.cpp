// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "applicationadaptor.h"
#include "dbus/applicationmanager1adaptor.h"
#include "applicationservice.h"
#include "dbus/AMobjectmanager1adaptor.h"
#include "systemdsignaldispatcher.h"
#include <QFile>
#include <QDBusMessage>
#include <unistd.h>

ApplicationManager1Service::~ApplicationManager1Service() = default;

ApplicationManager1Service::ApplicationManager1Service(std::unique_ptr<Identifier> ptr, QDBusConnection &connection) noexcept
    : m_identifier(std::move(ptr))
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

    if (!registerObjectToDBus(this, DDEApplicationManager1ObjectPath, ApplicationManagerInterface)) {
        std::terminate();
    }

    m_jobManager.reset(new (std::nothrow) JobManager1Service(this));

    if (!m_jobManager) {
        qCritical() << "new JobManager failed.";
        std::terminate();
    }

    auto &dispatcher = SystemdSignalDispatcher::instance();

    connect(&dispatcher, &SystemdSignalDispatcher::SystemdUnitNew, this, &ApplicationManager1Service::addInstanceToApplication);

    connect(&dispatcher,
            &SystemdSignalDispatcher::SystemdUnitRemoved,
            this,
            &ApplicationManager1Service::removeInstanceFromApplication);

    this->scanApplications();
    this->scanInstances();

    // TODO: send custom event;
}

void ApplicationManager1Service::addInstanceToApplication(const QString &unitName, const QDBusObjectPath &systemdUnitPath)
{
    if (!isApplication(systemdUnitPath)) {
        return;
    }

    auto pair = processUnitName(unitName);
    auto appId = pair.first;
    auto instanceId = pair.second;
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

    const auto &applicationPath = (*appIt)->applicationPath().path();

    if (!(*appIt)->addOneInstance(instanceId, applicationPath, systemdUnitPath.path())) [[likely]] {
        qCritical() << "add Instance failed:" << applicationPath << unitName << systemdUnitPath.path();
    }
}

void ApplicationManager1Service::removeInstanceFromApplication(const QString &unitName, const QDBusObjectPath &systemdUnitPath)
{
    if (!isApplication(systemdUnitPath)) {
        return;
    }

    auto pair = processUnitName(unitName);
    auto appId = pair.first;
    auto instanceId = pair.second;
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
            return value->systemdUnitPath() == systemdUnitPath;
        });

    if (instanceIt != appIns.cend()) [[likely]] {
        (*appIt)->removeOneInstance(instanceIt.key());
    }
}

void ApplicationManager1Service::scanApplications() noexcept
{
    QList<DesktopFile> fileList{};
    const auto &desktopFileDirs = getDesktopFileDirs();

    applyIteratively(QList<QDir>(desktopFileDirs.cbegin(), desktopFileDirs.cend()), [this](const QFileInfo &info) -> bool {
        DesktopErrorCode err{DesktopErrorCode::NoError};
        auto ret = DesktopFile::searchDesktopFileByPath(info.absoluteFilePath(), err);
        if (!ret.has_value()) {
            qWarning() << "failed to search File:" << err;
            return false;
        }
        if (!this->addApplication(std::move(ret).value())) {
            qWarning() << "add Application failed, skip...";
        }
        return false;  // means to apply this function to the rest of the files
    });
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

QList<QDBusObjectPath> ApplicationManager1Service::list() const
{
    return m_applicationList.keys();
}

bool ApplicationManager1Service::addApplication(DesktopFile desktopFileSource) noexcept
{
    QSharedPointer<ApplicationService> application =
        ApplicationService::createApplicationService(std::move(desktopFileSource), this);
    if (!application) {
        return false;
    }

    if (m_applicationList.constFind(application->applicationPath()) != m_applicationList.cend()) {
        qInfo() << "this application already exists."
                << "desktop source:" << application->desktopFileSource().sourcePath();
        return false;
    }

    auto *ptr = application.data();

    if (auto *adaptor = new (std::nothrow) ApplicationAdaptor{ptr}; adaptor == nullptr) {
        qCritical() << "new ApplicationAdaptor failed.";
        return false;
    }

    if (!registerObjectToDBus(ptr, application->applicationPath().path(), ApplicationInterface)) {
        return false;
    }
    m_applicationList.insert(application->applicationPath(), application);
    emit InterfacesAdded(application->applicationPath(), getChildInterfacesAndPropertiesFromObject(ptr));

    return true;
}

void ApplicationManager1Service::removeOneApplication(const QDBusObjectPath &application) noexcept
{
    if (auto it = m_applicationList.find(application); it != m_applicationList.cend()) {
        emit InterfacesRemoved(application, getChildInterfacesFromObject(it->data()));
        unregisterObjectFromDBus(application.path());
        m_applicationList.remove(application);
    }
}

void ApplicationManager1Service::removeAllApplication() noexcept
{
    for (const auto &app : m_applicationList.keys()) {
        removeOneApplication(app);
    }
}

QString ApplicationManager1Service::Identify(const QDBusUnixFileDescriptor &pidfd,
                                             QDBusObjectPath &application,
                                             QDBusObjectPath &application_instance)
{
    if (!pidfd.isValid()) {
        qWarning() << "pidfd isn't a valid unix file descriptor";
        return {};
    }

    Q_ASSERT_X(static_cast<bool>(m_identifier), "Identify", "Broken Identifier.");

    QString fdFilePath = QString{"/proc/self/fdinfo/%1"}.arg(pidfd.fileDescriptor());
    QFile fdFile{fdFilePath};
    if (!fdFile.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text)) {
        qWarning() << "open " << fdFilePath << "failed: " << fdFile.errorString();
        return {};
    }
    auto content = fdFile.readAll();
    QTextStream stream{content};
    QString appPid;
    while (!stream.atEnd()) {
        auto line = stream.readLine();
        if (line.startsWith("Pid")) {
            appPid = line.split(":").last().trimmed();
            break;
        }
    }
    if (appPid.isEmpty()) {
        qWarning() << "can't find pid which corresponding with the instance of this application.";
        return {};
    }
    bool ok;
    auto pid = appPid.toUInt(&ok);
    if (!ok) {
        qWarning() << "AppId is failed to convert to uint.";
        return {};
    }

    const auto ret = m_identifier->Identify(pid);

    auto app = std::find_if(m_applicationList.cbegin(), m_applicationList.cend(), [&ret](const auto &appPtr) {
        return appPtr->id() == ret.ApplicationId;
    });
    if (app == m_applicationList.cend()) {
        qWarning() << "can't find application:" << ret.ApplicationId;
        return {};
    }
    application = m_applicationList.key(*app);
    application_instance = (*app)->findInstance(ret.InstanceId);
    return ret.ApplicationId;
}

void ApplicationManager1Service::updateApplication(const QSharedPointer<ApplicationService> &destApp,
                                                   const DesktopFile &desktopFile) noexcept
{
    if (auto app = m_applicationList.find(destApp->applicationPath()); app == m_applicationList.cend()) {
        return;
    }

    auto mtime = getFileTimeInfo(QFileInfo{desktopFile.sourceFileRef()});

    if (destApp->desktopFileSource().modified(std::get<1>(mtime))) {
        auto *newEntry = new (std::nothrow) DesktopEntry{};
        if (newEntry == nullptr) {
            qCritical() << "new DesktopEntry failed.";
            return;
        }

        auto err = newEntry->parse(destApp->desktopFileSource());
        if (err != DesktopErrorCode::NoError) {
            qWarning() << "update desktop file failed:" << err << ", content wouldn't change.";
            return;
        }
        destApp->resetEntry(newEntry);
    }
}

void ApplicationManager1Service::UpdateApplicationInfo(const QStringList &appIdList)
{
    for (const auto &appId : appIdList) {
        DesktopErrorCode err{DesktopErrorCode::NotFound};
        auto file = DesktopFile::searchDesktopFileById(appId, err);
        auto destApp = std::find_if(m_applicationList.cbegin(),
                                    m_applicationList.cend(),
                                    [&appId](const QSharedPointer<ApplicationService> &app) { return appId == app->id(); });

        if (err == DesktopErrorCode::NotFound) {
            removeOneApplication(destApp.key());
            continue;
        }

        if (destApp != m_applicationList.cend()) {
            updateApplication(destApp.value(), file.value());
            continue;
        }

        addApplication(std::move(file).value());
    }
}

ObjectMap ApplicationManager1Service::GetManagedObjects() const
{
    return dumpDBusObject(m_applicationList);
}
