// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/jobservice.h"

JobService::JobService(const QFuture<QVariantList> &job)
    : m_job(job)
{
}

JobService::~JobService() = default;

QString JobService::status() const  // FIXME: job status aren't mutually exclusive
{
    if (m_job.isFinished()) {
        return "finished";
    }
    if (m_job.isCanceled()) {
        return "canceled";
    }
    if (m_job.isSuspended()) {
        return "suspended";
    }
    if (m_job.isSuspending()) {
        return "suspending";
    }
    if (m_job.isStarted()) {
        return "started";
    }
    if (m_job.isRunning()) {
        return "running";
    }
    Q_UNREACHABLE();
}

void JobService::Cancel()
{
    m_job.cancel();
}

void JobService::Suspend()
{
    m_job.suspend();
}

void JobService::Resume()
{
    m_job.resume();
}
