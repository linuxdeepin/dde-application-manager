// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONSERVICE_H
#define APPLICATIONSERVICE_H

#include <QObject>
#include <QDBusObjectPath>

class ApplicationService : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationService(QObject *parent = nullptr);
    virtual ~ApplicationService();

public Q_SLOTS:
    QDBusObjectPath Launch(const QString &action, const QStringList &fields, const QVariantMap &options);
};

#endif
