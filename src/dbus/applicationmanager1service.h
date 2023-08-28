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
#include "desktopentry.h"
#include "identifier.h"

class ApplicationService;

class ApplicationManager1Service final : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationManager1Service(std::unique_ptr<Identifier> ptr, QDBusConnection &connection) noexcept;
    ~ApplicationManager1Service() override;
    ApplicationManager1Service(const ApplicationManager1Service &) = delete;
    ApplicationManager1Service(ApplicationManager1Service &&) = delete;
    ApplicationManager1Service &operator=(const ApplicationManager1Service &) = delete;
    ApplicationManager1Service &operator=(ApplicationManager1Service &&) = delete;

    Q_PROPERTY(QList<QDBusObjectPath> List READ list)
    [[nodiscard]] QList<QDBusObjectPath> list() const;

    bool addApplication(DesktopFile desktopFileSource) noexcept;
    void removeOneApplication(const QDBusObjectPath &application) noexcept;
    void removeAllApplication() noexcept;

    void updateApplication(const QSharedPointer<ApplicationService> &destApp, const DesktopFile &desktopFile) noexcept;

    JobManager1Service &jobManager() noexcept { return *m_jobManager; }

public Q_SLOTS:
    QString Identify(const QDBusUnixFileDescriptor &pidfd, QDBusObjectPath &application, QDBusObjectPath &application_instance);
    void UpdateApplicationInfo(const QStringList &appIdList);
    [[nodiscard]] ObjectMap GetManagedObjects() const;

Q_SIGNALS:
    void InterfacesAdded(const QDBusObjectPath &object_path, const ObjectInterfaceMap &interfaces);
    void InterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces);

private:
    std::unique_ptr<Identifier> m_identifier;
    QScopedPointer<JobManager1Service> m_jobManager{nullptr};
    QMap<QDBusObjectPath, QSharedPointer<ApplicationService>> m_applicationList;

    void scanApplications() noexcept;
    void scanInstances() noexcept;
    void addInstanceToApplication(const QString &unitName, const QDBusObjectPath &systemdUnitPath);
    void removeInstanceFromApplication(const QString &unitName, const QDBusObjectPath &systemdUnitPath);
};

#endif
