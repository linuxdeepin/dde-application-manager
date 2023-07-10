// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef JOBMANAGER1SERVICE_H
#define JOBMANAGER1SERVICE_H

#include <QObject>
#include <QDBusObjectPath>

class JobManager1Service : public QObject
{
    Q_OBJECT
public:
    explicit JobManager1Service(QObject *parent = nullptr);
    virtual ~JobManager1Service();

Q_SIGNALS:
    void JobNew(const QDBusObjectPath &job, const QDBusObjectPath &source);
    void JobRemoved(const QDBusObjectPath &job, const QString &status, const QString &message, const QDBusVariant &result);
};

#endif
