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
    explicit InstanceService(QObject *parent = nullptr);
    virtual ~InstanceService();

public:
    Q_PROPERTY(QDBusObjectPath Application READ application)
    QDBusObjectPath application() const;

    Q_PROPERTY(QDBusObjectPath SystemdUnitPath READ systemdUnitPath)
    QDBusObjectPath systemdUnitPath() const;
};

#endif
