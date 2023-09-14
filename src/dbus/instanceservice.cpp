// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/instanceservice.h"
#include "propertiesForwarder.h"
#include <QCoreApplication>

InstanceService::InstanceService(QString instanceId, QString application, QString systemdUnitPath, QString launcher)
    : m_Launcher(std::move(launcher))
    , m_instanceId(std::move(instanceId))
    , m_Application(std::move(application))
    , m_SystemdUnitPath(std::move(systemdUnitPath))
{
    new PropertiesForwarder{application + "/" + instanceId, this};
}

InstanceService::~InstanceService() = default;
