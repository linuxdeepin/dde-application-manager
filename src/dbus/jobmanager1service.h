// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef JOBMANAGER1SERVICE_H
#define JOBMANAGER1SERVICE_H

#include <QObject>
#include <QMap>
#include <QDBusObjectPath>
#include <QSharedPointer>
#include "jobservice.h"
class JobManager1Service final : public QObject
{
    Q_OBJECT
public:
    JobManager1Service();
    JobManager1Service(const JobManager1Service &) = delete;
    JobManager1Service(JobManager1Service&&) = delete;
    JobManager1Service &operator=(const JobManager1Service &) = delete;
    JobManager1Service &operator=(JobManager1Service &&) = delete;

    ~JobManager1Service() override;
    template<typename F,typename ...Args>
    void addJob(const QDBusObjectPath &source,F func, Args&& ...args);
    bool removeJob(const QDBusObjectPath &job, const QString &status, const QString &message, const QDBusVariant &result);

Q_SIGNALS:
    void JobNew(const QDBusObjectPath &job, const QDBusObjectPath &source);
    void JobRemoved(const QDBusObjectPath &job, const QString &status, const QString &message, const QDBusVariant &result);

private:
    QMap<QDBusObjectPath, QSharedPointer<JobService>> m_jobs;
};

#endif
