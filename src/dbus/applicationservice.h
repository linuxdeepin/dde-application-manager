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
#include "instanceservice.h"
#include "global.h"

class ApplicationService : public QObject
{
    Q_OBJECT
public:
    ~ApplicationService() override;
    ApplicationService(const ApplicationService &) = delete;
    ApplicationService(ApplicationService &&) = delete;
    ApplicationService &operator=(const ApplicationService &) = delete;
    ApplicationService &operator=(ApplicationService &&) = delete;

    Q_PROPERTY(QStringList Actions READ actions CONSTANT)
    QStringList actions() const noexcept;
    QStringList &actionsRef() noexcept;

    Q_PROPERTY(QString ID READ iD CONSTANT)
    QString iD() const noexcept;

    Q_PROPERTY(IconMap Icons READ icons)
    IconMap icons() const;
    IconMap &iconsRef();

    Q_PROPERTY(bool AutoStart READ isAutoStart WRITE setAutoStart)
    bool isAutoStart() const noexcept;
    void setAutoStart(bool autostart) noexcept;

    Q_PROPERTY(QList<QDBusObjectPath> Instances READ instances)
    QList<QDBusObjectPath> instances() const noexcept;

    void recoverInstances(const QList<QDBusObjectPath>) noexcept;
    bool removeOneInstance(const QDBusObjectPath &instance);
    void removeAllInstance();

public Q_SLOTS:
    QString GetActionName(const QString &identifier, const QStringList &env);
    QDBusObjectPath Launch(const QString &action, const QStringList &fields, const QVariantMap &options);

private:
    friend class ApplicationManager1Service;
    explicit ApplicationService(QString ID);
    bool m_AutoStart;
    QString m_ID;
    QDBusObjectPath m_applicationPath;
    QStringList m_actions;
    QMap<QDBusObjectPath, QSharedPointer<InstanceService>> m_Instances;
    IconMap m_Icons;
};

#endif
