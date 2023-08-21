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
#include <QFile>
#include <memory>
#include <utility>
#include "dbus/instanceservice.h"
#include "global.h"
#include "desktopentry.h"
#include "desktopicons.h"
#include "dbus/jobmanager1service.h"

class ApplicationService : public QObject
{
    Q_OBJECT
public:
    ~ApplicationService() override;
    ApplicationService(const ApplicationService &) = delete;
    ApplicationService(ApplicationService &&) = delete;
    ApplicationService &operator=(const ApplicationService &) = delete;
    ApplicationService &operator=(ApplicationService &&) = delete;

    Q_PROPERTY(QStringList Actions READ actions)
    [[nodiscard]] QStringList actions() const noexcept;

    Q_PROPERTY(QString ID READ id CONSTANT)
    [[nodiscard]] QString id() const noexcept;

    Q_PROPERTY(IconMap Icons READ icons)
    [[nodiscard]] IconMap icons() const;
    IconMap &iconsRef();

    Q_PROPERTY(bool AutoStart READ isAutoStart WRITE setAutoStart)
    [[nodiscard]] bool isAutoStart() const noexcept;
    void setAutoStart(bool autostart) noexcept;

    Q_PROPERTY(QList<QDBusObjectPath> Instances READ instances)
    [[nodiscard]] QList<QDBusObjectPath> instances() const noexcept;

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
    [[nodiscard]] QString GetActionName(const QString &identifier, const QStringList &env) const;
    QDBusObjectPath Launch(const QString &action, const QStringList &fields, const QVariantMap &options);
    [[nodiscard]] QString GetIconName(const QString &action) const;
    [[nodiscard]] QString GetDisplayName(const QStringList &env) const;
    [[nodiscard]] ObjectMap GetManagedObjects() const;

Q_SIGNALS:
    void InterfacesAdded(const QDBusObjectPath &object_path, const QStringList &interfaces);
    void InterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces);

private:
    friend class ApplicationManager1Service;
    explicit ApplicationService(DesktopFile source, ApplicationManager1Service *parent);
    static QSharedPointer<ApplicationService> createApplicationService(DesktopFile source,
                                                                       ApplicationManager1Service *parent) noexcept;
    bool m_AutoStart{false};
    QDBusObjectPath m_applicationPath;
    QString m_launcher{getApplicationLauncherBinary()};
    DesktopFile m_desktopSource;
    QSharedPointer<DesktopEntry> m_entry{nullptr};
    QSharedPointer<DesktopIcons> m_Icons{nullptr};
    QMap<QDBusObjectPath, QSharedPointer<InstanceService>> m_Instances;
    static QString userNameLookup(uid_t uid);
    [[nodiscard]] LaunchTask unescapeExec(const QString &str, const QStringList &fields);
};

#endif
