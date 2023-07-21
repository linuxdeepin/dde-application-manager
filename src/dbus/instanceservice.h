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

    Q_PROPERTY(QDBusObjectPath Application READ application CONSTANT)
    QDBusObjectPath application() const;

    Q_PROPERTY(QDBusObjectPath SystemdUnitPath READ systemdUnitPath CONSTANT)
    QDBusObjectPath systemdUnitPath() const;

private:
    friend class ApplicationService;
    InstanceService(QString application, QString systemdUnitPath);
    const QDBusObjectPath m_Application;
    const QDBusObjectPath m_SystemdUnitPath;
};

#endif
