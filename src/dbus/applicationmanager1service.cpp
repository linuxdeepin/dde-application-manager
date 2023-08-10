// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "dbus/applicationmanager1service.h"
#include "dbus/applicationmanager1adaptor.h"
#include "systemdsignaldispatcher.h"
#include <QFile>
#include <unistd.h>

ApplicationManager1Service::~ApplicationManager1Service() = default;

ApplicationManager1Service::ApplicationManager1Service(std::unique_ptr<Identifier> ptr, QDBusConnection &connection)
    : m_identifier(std::move(ptr))
{
    if (!connection.registerService(DDEApplicationManager1ServiceName)) {
        qFatal() << connection.lastError();
    }

    if (!registerObjectToDBus(new ApplicationManager1Adaptor{this},
                              DDEApplicationManager1ObjectPath,
                              getDBusInterface<ApplicationManager1Adaptor>())) {
        std::terminate();
    }

    m_jobManager.reset(new JobManager1Service(this));

    auto &dispatcher = SystemdSignalDispatcher::instance();

    connect(&dispatcher,
            &SystemdSignalDispatcher::SystemdUnitNew,
            this,
            [this](const QString &serviceName, const QDBusObjectPath &systemdUnitPath) {
                auto [appId, instanceId] = processServiceName(serviceName);
                if (appId.isEmpty()) {
                    return;
                }

                for (const auto &app : m_applicationList) {
                    if (app->id() == appId) [[unlikely]] {
                        const auto &applicationPath = app->m_applicationPath.path();
                        if (!app->addOneInstance(instanceId, applicationPath, systemdUnitPath.path())) {
                            qWarning() << "add Instance failed:" << applicationPath << serviceName << systemdUnitPath.path();
                        }
                        return;
                    }
                }

                qWarning() << "couldn't find application:" << serviceName << "in application manager.";
            });

    connect(&dispatcher,
            &SystemdSignalDispatcher::SystemdUnitRemoved,
            this,
            [this](const QString &serviceName, QDBusObjectPath systemdUnitPath) {
                auto pair = processServiceName(serviceName);
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

                const auto &appRef = *appIt;

                auto instanceIt = std::find_if(appRef->m_Instances.cbegin(),
                                               appRef->m_Instances.cend(),
                                               [&systemdUnitPath](const QSharedPointer<InstanceService> &value) {
                                                   return value->systemdUnitPath() == systemdUnitPath;
                                               });

                if (instanceIt != appRef->m_Instances.cend()) [[likely]] {
                    appRef->removeOneInstance(instanceIt.key());
                }
            });
}

QPair<QString, QString> ApplicationManager1Service::processServiceName(const QString &serviceName) noexcept
{
    QString instanceId;
    QString applicationId;

    if (serviceName.endsWith(".service")) {
        auto lastDotIndex = serviceName.lastIndexOf('.');
        auto app = serviceName.sliced(0, lastDotIndex - 1);  // remove suffix

        if (app.contains('@')) {
            auto atIndex = app.indexOf('@');
            instanceId = app.sliced(atIndex + 1);
            app.remove(atIndex, instanceId.length() + 1);
        }

        applicationId = app.split('-').last();  // drop launcher if it exists.
    } else if (serviceName.endsWith(".scope")) {
        auto lastDotIndex = serviceName.lastIndexOf('.');
        auto app = serviceName.sliced(0, lastDotIndex - 1);

        auto components = app.split('-');
        instanceId = components.takeLast();
        applicationId = components.takeLast();
    } else {
        qDebug() << "it's not service or slice or scope.";
        return {};
    }

    if (instanceId.isEmpty()) {
        instanceId = QUuid::createUuid().toString(QUuid::Id128);
    }

    return qMakePair(std::move(applicationId), std::move(instanceId));
}

QList<QDBusObjectPath> ApplicationManager1Service::list() const
{
    return m_applicationList.keys();
}

void ApplicationManager1Service::removeOneApplication(const QDBusObjectPath &application)
{
    unregisterObjectFromDBus(application.path());
    m_applicationList.remove(application);
}

void ApplicationManager1Service::removeAllApplication()
{
    for (const auto &app : m_applicationList.keys()) {
        removeOneApplication(app);
    }
}

QDBusObjectPath ApplicationManager1Service::Application(const QString &id) const
{
    auto ret = std::find_if(m_applicationList.cbegin(), m_applicationList.cend(), [&id](const auto &app) {
        return static_cast<bool>(app->id() == id);
    });

    if (ret != m_applicationList.cend()) {
        return ret.key();
    }
    return {};
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

QDBusObjectPath ApplicationManager1Service::Launch(const QString &id,
                                                   const QString &actions,
                                                   const QStringList &fields,
                                                   const QVariantMap &options)
{
    auto app = Application(id);
    if (app.path().isEmpty()) {
        qWarning() << "No such application.";
        return {};
    }
    const auto &value = m_applicationList.value(app);
    return value->Launch(actions, fields, options);
}

void ApplicationManager1Service::UpdateApplicationInfo(const QStringList &app_id)
{
    auto XDGDataDirs = QString::fromLocal8Bit(qgetenv("XDG_DATA_DIRS")).split(':', Qt::SkipEmptyParts);
    std::for_each(XDGDataDirs.begin(), XDGDataDirs.end(), [](QString &str) {
        if (!str.endsWith(QDir::separator())) {
            str.append(QDir::separator());
        }
        str.append("applications");
    });

    for (auto id : app_id) {
        auto destApp = std::find_if(m_applicationList.begin(),
                                    m_applicationList.end(),
                                    [&id](const QSharedPointer<ApplicationService> &value) { return value->id() == id; });

        if (destApp == m_applicationList.end()) {  // new app
            qInfo() << "add a new application:" << id;
            do {
                for (const auto &suffix : XDGDataDirs) {
                    QFileInfo info{suffix + id};
                    if (info.exists()) {
                        ParseError err;
                        auto file = DesktopFile::searchDesktopFile(info.absoluteFilePath(), err);

                        if (!file.has_value()) {
                            continue;
                        }

                        if (!addApplication(std::move(file).value())) {
                            id.clear();
                            break;
                        }
                    }
                }

                if (id.isEmpty()) {
                    break;
                }

                auto hyphenIndex = id.indexOf('-');
                if (hyphenIndex == -1) {
                    break;
                }

                id[hyphenIndex] = QDir::separator();
            } while (true);
        } else {  // remove or update
            if (!(*destApp)->m_isPersistence) [[unlikely]] {
                continue;
            }

            auto filePath = (*destApp)->m_desktopSource.m_file.filePath();
            if (QFileInfo::exists(filePath)) {  // update
                qInfo() << "update application:" << id;
                struct stat buf;
                if (auto ret = stat(filePath.toLatin1().data(), &buf); ret == -1) {
                    qWarning() << "get file" << filePath << "state failed:" << std::strerror(errno) << ", skip...";
                    continue;
                }

                if ((*destApp)->m_desktopSource.m_file.modified(
                        static_cast<std::size_t>(buf.st_mtim.tv_sec * 1e9 + buf.st_mtim.tv_nsec))) {
                    auto newEntry = new DesktopEntry{};
                    auto err = newEntry->parse((*destApp)->m_desktopSource.m_file);
                    if (err != ParseError::NoError and err != ParseError::EntryKeyInvalid) {
                        qWarning() << "update desktop file failed:" << err << ", content wouldn't change.";
                        continue;
                    }
                    (*destApp)->m_entry.reset(newEntry);
                }
            } else {  // remove
                qInfo() << "remove application:" << id;
                removeOneApplication((*destApp)->m_applicationPath);
            }
        }
    }
}
