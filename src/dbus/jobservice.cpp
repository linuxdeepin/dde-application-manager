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
    // TODO: impl
    return {};
}

void JobService::Cancel()
{
    // TODO: impl
}

void JobService::Pause()
{
    // TODO: impl
}

void JobService::Resume()
{
    // TODO: impl
}
