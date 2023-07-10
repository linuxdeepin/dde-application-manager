// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "instanceservice.h"

InstanceService::InstanceService(QObject *parent)
    : QObject(parent)
{
}

InstanceService::~InstanceService() {}

QDBusObjectPath InstanceService::application() const
{
    // TODO: impl
    return {};
}

QDBusObjectPath InstanceService::systemdUnitPath() const
{
    // TODO: impl
    return {};
}
