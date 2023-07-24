// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef INSTANCESERVICE_H
#define INSTANCESERVICE_H

#include <QObject>
#include <QDBusObjectPath>

class InstanceService : public QObject
{
    Q_OBJECT
public:
    ~InstanceService() override;
    InstanceService(const InstanceService &) = delete;
    InstanceService(InstanceService &&) = delete;
    InstanceService &operator=(const InstanceService &) = delete;
    InstanceService &operator=(InstanceService &&) = delete;

    Q_PROPERTY(QDBusObjectPath Application READ application)
    QDBusObjectPath application() const;

    Q_PROPERTY(QDBusObjectPath SystemdUnitPath READ systemdUnitPath)
    QDBusObjectPath systemdUnitPath() const;

    const QString &instanceId() const noexcept { return m_instanceId; }

private:
    friend class ApplicationService;
    InstanceService(QString instanceId, QString application, QString systemdUnitPath);
    QString m_instanceId;
    const QDBusObjectPath m_Application;
    const QDBusObjectPath m_SystemdUnitPath;
};

#endif
