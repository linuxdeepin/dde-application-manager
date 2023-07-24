// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "applicationmanager1service.h"
#include "applicationmanager1adaptor.h"
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
}

QList<QDBusObjectPath> ApplicationManager1Service::list() const
{
    return m_applicationList.keys();
}

bool ApplicationManager1Service::removeOneApplication(const QDBusObjectPath &application)
{
    return m_applicationList.remove(application) != 0;
}

void ApplicationManager1Service::removeAllApplication()
{
    m_applicationList.clear();
}

QDBusObjectPath ApplicationManager1Service::Application(const QString &id) const
{
    auto ret = std::find_if(m_applicationList.cbegin(), m_applicationList.cend(), [&id](const auto &app) {
        if (app->id() == id) {
            return true;
        }
        return false;
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

void ApplicationManager1Service::UpdateApplicationInfo(const QStringList &update_path) {}
