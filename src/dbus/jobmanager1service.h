// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef JOBMANAGER1SERVICE_H
#define JOBMANAGER1SERVICE_H

#include <QObject>
#include <QMap>
#include <QDBusObjectPath>
#include <QSharedPointer>
#include <QMutex>
#include <QMutexLocker>
#include <QtConcurrent>
#include <QFuture>
#include <QUuid>
#include "jobadaptor.h"
#include "global.h"

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
    template <typename F>
    void addJob(const QDBusObjectPath &source, F func, QVariantList args)
    {
        static_assert(std::is_invocable_v<F, QVariant>, "param type must be QVariant.");

        QString objectPath{DDEApplicationManager1JobObjectPath + QUuid::createUuid().toString(QUuid::Id128)};
        auto future = QtConcurrent::mappedReduced(
            args.begin(), args.end(), func,qOverload<QVariantList::parameter_type>(&QVariantList::append), QVariantList{}, QtConcurrent::ReduceOption::OrderedReduce);
        QSharedPointer<JobService> job{new JobService{future}};
        auto path = QDBusObjectPath{objectPath};
        {
            QMutexLocker locker{&m_mutex};
            m_jobs.insert(path, job);  // Insertion is always successful
        }
        emit JobNew(path, source);
        registerObjectToDbus(new JobAdaptor(job.data()), objectPath, getDBusInterface<JobAdaptor>());
        auto emitRemove = [this, job, path, future] (QVariantList value) {
            decltype(m_jobs)::size_type removeCount{0};
            {
                QMutexLocker locker{&m_mutex};
                removeCount = m_jobs.remove(path);
            }
            // removeCount means m_jobs can't find value which corresponding with path
            // and we shouldn't emit jobRemoved signal because this signal may already has been emit
            if (removeCount == 0) {
                qCritical() << "Job already has been removed: " << path.path();
                return value;
            }
            QString result{job->status()};
            for (const auto &val : future.result()) {
                if (val.template canConvert<QDBusError>()) {
                    result = "failed";
                }
                break;
            }
            emit this->JobRemoved(path, result, future.result());
            return value;
        };
        future.then(emitRemove);
    }

Q_SIGNALS:
    void JobNew(const QDBusObjectPath &job, const QDBusObjectPath &source);
    void JobRemoved(const QDBusObjectPath &job, const QString &status, const QVariantList &result);

private:
    QMutex m_mutex;
    QMap<QDBusObjectPath, QSharedPointer<JobService>> m_jobs;
};

#endif
