// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "jobmanager1service.h"
#include <QUuid>
#include "global.h"

JobManager1Service::JobManager1Service() = default;

JobManager1Service::~JobManager1Service() = default;

template <typename F, typename... Args>
void addJob(const QDBusObjectPath &source, F func, Args &&...args)
{
    // TODO: impl
}

bool JobManager1Service::removeJob(const QDBusObjectPath &job,
                                   const QString &status,
                                   const QString &message,
                                   const QDBusVariant &result)
{
    // TODO: impl
    return false;
}
