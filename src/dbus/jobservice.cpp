// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "jobservice.h"

JobService::JobService(QObject *parent)
    : QObject(parent)
{
}

JobService::~JobService() {}

QString JobService::status() const
{
    if (job.isFinished())
        return "finished";
    if (job.isCanceled())
        return "canceled";
    if (job.isSuspended())
        return "suspended";
    if (job.isSuspending())
        return "suspending";
    if (job.isStarted())
        return "started";
    if (job.isRunning())
        return "running";
    return "failed";
}

void JobService::Cancel()
{
    job.cancel();
}

void JobService::Pause()
{
    job.suspend();
}

void JobService::Resume()
{
    job.resume();
}
