// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationservice.h"
#include "applicationadaptor.h"
#include "instanceadaptor.h"
#include <QUuid>

ApplicationService::ApplicationService(QString ID)
    : m_AutoStart(false)
    , m_ID(std::move(ID))
    , m_applicationPath(DDEApplicationManager1ApplicationObjectPath + m_ID)
{
}

ApplicationService::~ApplicationService() = default;

QString ApplicationService::GetActionName(const QString &identifier, const QStringList &env)
{
    // TODO: impl
    return {};
}

QDBusObjectPath ApplicationService::Launch(const QString &action, const QStringList &fields, const QVariantMap &options)
{
    // TODO: impl launch app from systemd.
    QString objectPath{m_applicationPath.path() + "/" + QUuid::createUuid().toString()};
    QSharedPointer<InstanceService> ins{new InstanceService{objectPath, ""}};
    auto *ptr = ins.data();
    if (registerObjectToDbus(new InstanceAdaptor(ptr), objectPath, getDBusInterface<InstanceAdaptor>())) {
        m_Instances.insert(QDBusObjectPath{objectPath}, ins);
        return QDBusObjectPath{objectPath};
    }
    return {};
}

QStringList ApplicationService::actions() const noexcept
{
    return m_actions;
}

QStringList &ApplicationService::actionsRef() noexcept
{
    return m_actions;
}

QString ApplicationService::iD() const noexcept
{
    return m_ID;
}

IconMap ApplicationService::icons() const
{
    return m_Icons;
}

IconMap &ApplicationService::iconsRef()
{
    return m_Icons;
}

bool ApplicationService::isAutoStart() const noexcept
{
    return m_AutoStart;
}

void ApplicationService::setAutoStart(bool autostart) noexcept
{
    m_AutoStart = autostart;
}

QList<QDBusObjectPath> ApplicationService::instances() const noexcept
{
    return m_Instances.keys();
}

bool ApplicationService::removeOneInstance(const QDBusObjectPath &instance)
{
    return m_Instances.remove(instance) != 0;
}

void ApplicationService::removeAllInstance()
{
    m_Instances.clear();
}
