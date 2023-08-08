// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONMANAGER1SERVICE_H
#define APPLICATIONMANAGER1SERVICE_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QSharedPointer>
#include <QScopedPointer>
#include <memory>
#include <QMap>
#include "dbus/jobmanager1service.h"
#include "dbus/applicationservice.h"
#include "dbus/applicationadaptor.h"
#include "identifier.h"

class ApplicationManager1Service final : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationManager1Service(std::unique_ptr<Identifier> ptr, QDBusConnection &connection);
    ~ApplicationManager1Service() override;
    ApplicationManager1Service(const ApplicationManager1Service &) = delete;
    ApplicationManager1Service(ApplicationManager1Service &&) = delete;
    ApplicationManager1Service &operator=(const ApplicationManager1Service &) = delete;
    ApplicationManager1Service &operator=(ApplicationManager1Service &&) = delete;

    Q_PROPERTY(QList<QDBusObjectPath> List READ list)
    QList<QDBusObjectPath> list() const;
    template <typename T>
    bool addApplication(T &&desktopFileSource)
    {
        QSharedPointer<ApplicationService> application{new ApplicationService{std::forward<T>(desktopFileSource), this}};
        if (!application) {
            return false;
        }
        if (!registerObjectToDBus(new ApplicationAdaptor{application.data()},
                                  application->m_applicationPath.path(),
                                  getDBusInterface<ApplicationAdaptor>())) {
            return false;
        }
        m_applicationList.insert(application->m_applicationPath, application);
        return true;
    }
    bool removeOneApplication(const QDBusObjectPath &application);
    void removeAllApplication();

    JobManager1Service &jobManager() noexcept { return *m_jobManager; }

public Q_SLOTS:
    QDBusObjectPath Application(const QString &id) const;
    QString Identify(const QDBusUnixFileDescriptor &pidfd, QDBusObjectPath &application, QDBusObjectPath &application_instance);
    QDBusObjectPath Launch(const QString &id, const QString &action, const QStringList &fields, const QVariantMap &options);
    void UpdateApplicationInfo(const QStringList &update_path);

private:
    std::unique_ptr<Identifier> m_identifier;
    QScopedPointer<JobManager1Service> m_jobManager{nullptr};
    QMap<QDBusObjectPath, QSharedPointer<ApplicationService>> m_applicationList;

    QPair<QString, QString> processServiceName(const QString &serviceName);
};

#endif
