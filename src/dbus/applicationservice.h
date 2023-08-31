// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONSERVICE_H
#define APPLICATIONSERVICE_H

#include <QObject>
#include <QDBusObjectPath>
#include <QMap>
#include <QString>
#include <QDBusUnixFileDescriptor>
#include <QSharedPointer>
#include <QUuid>
#include <QTextStream>
#include <QDBusContext>
#include <QFile>
#include <memory>
#include <utility>
#include "dbus/instanceservice.h"
#include "global.h"
#include "desktopentry.h"
#include "dbus/jobmanager1service.h"

class ApplicationService : public QObject, public QDBusContext
{
    Q_OBJECT
public:
    ~ApplicationService() override;
    ApplicationService(const ApplicationService &) = delete;
    ApplicationService(ApplicationService &&) = delete;
    ApplicationService &operator=(const ApplicationService &) = delete;
    ApplicationService &operator=(ApplicationService &&) = delete;

    Q_PROPERTY(QStringList Categories READ categories)
    [[nodiscard]] QStringList categories() const noexcept;

    Q_PROPERTY(QStringList Actions READ actions)
    [[nodiscard]] QStringList actions() const noexcept;

    Q_PROPERTY(PropMap ActionName READ actionName)
    [[nodiscard]] PropMap actionName() const noexcept;

    Q_PROPERTY(QString ID READ id CONSTANT)
    [[nodiscard]] QString id() const noexcept;

    Q_PROPERTY(PropMap DisplayName READ displayName)
    [[nodiscard]] PropMap displayName() const noexcept;

    Q_PROPERTY(PropMap Icons READ icons)
    [[nodiscard]] PropMap icons() const noexcept;

    Q_PROPERTY(qulonglong LastLaunchedTime READ lastLaunchedTime)
    [[nodiscard]] qulonglong lastLaunchedTime() const noexcept;

    Q_PROPERTY(bool AutoStart READ isAutoStart WRITE setAutoStart)
    [[nodiscard]] bool isAutoStart() const noexcept;
    void setAutoStart(bool autostart) noexcept;

    Q_PROPERTY(QList<QDBusObjectPath> Instances READ instances)
    [[nodiscard]] QList<QDBusObjectPath> instances() const noexcept;

    Q_PROPERTY(bool X_Flatpak READ x_Flatpak)
    [[nodiscard]] bool x_Flatpak() const noexcept;

    Q_PROPERTY(bool X_linglong READ x_linglong)
    [[nodiscard]] bool x_linglong() const noexcept;

    Q_PROPERTY(qulonglong installedTime READ installedTime)
    [[nodiscard]] qulonglong installedTime() const noexcept;

    [[nodiscard]] QDBusObjectPath findInstance(const QString &instanceId) const;

    [[nodiscard]] const QString &getLauncher() const noexcept { return m_launcher; }
    void setLauncher(const QString &launcher) noexcept { m_launcher = launcher; }

    bool addOneInstance(const QString &instanceId, const QString &application, const QString &systemdUnitPath);
    void recoverInstances(const QList<QDBusObjectPath> &instanceList) noexcept;
    void removeOneInstance(const QDBusObjectPath &instance) noexcept;
    void removeAllInstance() noexcept;
    [[nodiscard]] const QDBusObjectPath &applicationPath() const noexcept { return m_applicationPath; }
    [[nodiscard]] DesktopFile &desktopFileSource() noexcept { return m_desktopSource; }
    [[nodiscard]] const QMap<QDBusObjectPath, QSharedPointer<InstanceService>> &applicationInstances() const noexcept
    {
        return m_Instances;
    }
    void resetEntry(DesktopEntry *newEntry) noexcept;

public Q_SLOTS:
    QDBusObjectPath Launch(const QString &action, const QStringList &fields, const QVariantMap &options);
    [[nodiscard]] ObjectMap GetManagedObjects() const;

Q_SIGNALS:
    void InterfacesAdded(const QDBusObjectPath &object_path, const ObjectInterfaceMap &interfaces);
    void InterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces);

private:
    friend class ApplicationManager1Service;
    explicit ApplicationService(DesktopFile source, ApplicationManager1Service *parent);
    static QSharedPointer<ApplicationService> createApplicationService(DesktopFile source,
                                                                       ApplicationManager1Service *parent) noexcept;
    bool m_AutoStart{false};
    qlonglong m_lastLaunch{0};
    QDBusObjectPath m_applicationPath;
    QString m_launcher{getApplicationLauncherBinary()};
    DesktopFile m_desktopSource;
    QSharedPointer<DesktopEntry> m_entry{nullptr};
    QMap<QDBusObjectPath, QSharedPointer<InstanceService>> m_Instances;
    [[nodiscard]] LaunchTask unescapeExec(const QString &str, const QStringList &fields);
    [[nodiscard]] QVariant findEntryValue(const QString &group,
                                          const QString &valueKey,
                                          EntryValueType type,
                                          const QLocale &locale = getUserLocale()) const noexcept;
};

#endif
