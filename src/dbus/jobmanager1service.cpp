// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "jobmanager1service.h"
#include "jobmanager1adaptor.h"

LaunchTask::LaunchTask()
{
    qRegisterMetaType<LaunchTask>();
}

JobManager1Service::JobManager1Service(ApplicationManager1Service *parent)
    : m_parent(parent)
{
    if (!registerObjectToDBus(
            new JobManager1Adaptor{this}, DDEApplicationManager1JobManagerObjectPath, getDBusInterface<JobManager1Adaptor>())) {
        std::terminate();
    }
}

JobManager1Service::~JobManager1Service() = default;

bool JobManager1Service::removeOneJob(const QDBusObjectPath &path)
{
    decltype(m_jobs)::size_type removeCount{0};
    {
        QMutexLocker locker{&m_mutex};
        removeCount = m_jobs.remove(path);
    }
    // removeCount means m_jobs can't find value which corresponding with path
    // and we shouldn't emit jobRemoved signal because this signal may already has been emit
    if (removeCount == 0) {
        qWarning() << "Job already has been removed: " << path.path();
        return false;
    }

    unregisterObjectFromDBus(path.path());
    return true;
}
