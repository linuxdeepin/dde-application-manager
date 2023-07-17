// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "jobservice.h"

JobService::JobService(const QFuture<QVariant>& job)
    : m_job(job)
{
}

JobService::~JobService() = default;

QString JobService::status() const
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
    return "failed";
}

void JobService::Cancel()
{
    m_job.cancel();
}

void JobService::Pause()
{
    m_job.suspend();
}

void JobService::Resume()
{
    m_job.resume();
}
