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

    Q_PROPERTY(QString IconName READ iconName CONSTANT)
    [[nodiscard]] QString iconName() const noexcept;

    Q_PROPERTY(QString DisplayName READ displayName CONSTANT)
    [[nodiscard]] QString displayName() const noexcept;

    [[nodiscard]] QDBusObjectPath findInstance(const QString &instanceId) const;

    [[nodiscard]] const QString &getLauncher() const noexcept { return m_launcher; }
    void setLauncher(const QString &launcher) noexcept { m_launcher = launcher; }

    bool addOneInstance(const QString &instanceId, const QString &application, const QString &systemdUnitPath);
    void recoverInstances(const QList<QDBusObjectPath>) noexcept;
    void removeOneInstance(const QDBusObjectPath &instance);
    void removeAllInstance();

public Q_SLOTS:
    QString GetActionName(const QString &identifier, const QStringList &env);
    QDBusObjectPath Launch(QString action, QStringList fields, QVariantMap options);
    [[nodiscard]] ObjectMap GetManagedObjects() const;

Q_SIGNALS:
    void InterfacesAdded(const QDBusObjectPath &object_path, const QStringList &interfaces);
    void InterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces);

private:
    friend class ApplicationManager1Service;
    template <typename T>
    friend QSharedPointer<ApplicationService> makeApplication(T &&source, ApplicationManager1Service *parent);

    template <typename T>
    explicit ApplicationService(T &&source)
        : m_isPersistence(static_cast<bool>(std::is_same_v<T, DesktopFile>))
        , m_desktopSource(std::forward<T>(source))
    {
    }

    bool m_AutoStart{false};
    bool m_isPersistence;
    ApplicationManager1Service *m_parent{nullptr};
    QDBusObjectPath m_applicationPath;
    QString m_launcher{getApplicationLauncherBinary()};
    union DesktopSource
    {
        template <typename T, std::enable_if_t<std::is_same_v<T, DesktopFile>, bool> = true>
        DesktopSource(T &&source)
            : m_file(std::forward<T>(source))
        {
        }

        template <typename T, std::enable_if_t<std::is_same_v<T, QString>, bool> = true>
        DesktopSource(T &&source)
            : m_temp(std::forward<T>(source))
        {
        }

        void destruct(bool isPersistence)
        {
            if (isPersistence) {
                m_file.~DesktopFile();
            } else {
                m_temp.~QString();
            }
        }
        ~DesktopSource(){};
        QString m_temp;
        DesktopFile m_file;
    } m_desktopSource;
    QSharedPointer<DesktopEntry> m_entry{nullptr};
    QSharedPointer<DesktopIcons> m_Icons{nullptr};
    QMap<QDBusObjectPath, QSharedPointer<InstanceService>> m_Instances;
    QString userNameLookup(uid_t uid);
    [[nodiscard]] LaunchTask unescapeExec(const QString &str, const QStringList &fields);
};

#endif
