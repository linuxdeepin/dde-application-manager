// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "systemdsignaldispatcher.h"

bool SystemdSignalDispatcher::connectToSignals() noexcept
{
    using namespace Qt::StringLiterals;
    auto &con = ApplicationManager1DBus::instance().globalDestBus();

    if (!con.connect(SystemdService,
                     SystemdObjectPath,
                     SystemdInterfaceName,
                     u"UnitNew"_s,
                     this,
                     SLOT(onUnitNew(const QString &, const QDBusObjectPath &)))) {
        qCritical() << "can't connect to UnitNew signal of systemd service.";
        return false;
    }

    if (!con.connect(SystemdService,
                     SystemdObjectPath,
                     SystemdInterfaceName,
                     u"UnitRemoved"_s,
                     this,
                     SLOT(onUnitRemoved(const QString &, const QDBusObjectPath &)))) {
        qCritical() << "can't connect to UnitRemoved signal of systemd service.";
        return false;
    }

    if (!con.connect(SystemdService,
                     SystemdObjectPath,
                     SystemdInterfaceName,
                     u"PropertiesChanged"_s,
                     this,
                     SLOT(onPropertiesChanged(const QString &, const QVariantMap &, const QStringList &)))) {
        qCritical() << "can't connect to PropertiesChanged signal of systemd service.";
        return false;
    }

    if (!con.connect(SystemdService,
                     SystemdObjectPath,
                     SystemdInterfaceName,
                     u"JobNew"_s,
                     this,
                     SLOT(onJobNew(uint32_t, const QDBusObjectPath &, const QString &)))) {
        qCritical() << "can't connect to JobNew signal of systemd service.";
        return false;
    }

    return true;
}

void SystemdSignalDispatcher::onPropertiesChanged(const QString &interface,
                                                  const QVariantMap &props,
                                                  [[maybe_unused]] const QStringList &invalid)
{
    if (QStringView{interface} != SystemdPropInterfaceName) {
        return;
    }

    using namespace Qt::StringLiterals;
    if (auto it = props.constFind(u"Environment"_s); it != props.cend()) {
        emit SystemdEnvironmentChanged(it->toStringList());
    }
}

void SystemdSignalDispatcher::onUnitNew(const QString &unitName, const QDBusObjectPath &systemdUnitPath)
{
    emit SystemdUnitNew(unitName, systemdUnitPath);
}

void SystemdSignalDispatcher::onJobNew([[maybe_unused]] uint32_t id,
                                       const QDBusObjectPath &systemdUnitPath,
                                       const QString &unitName)
{
    emit SystemdJobNew(unitName, systemdUnitPath);
}

void SystemdSignalDispatcher::onUnitRemoved(const QString &unitName, const QDBusObjectPath &systemdUnitPath)
{
    emit SystemdUnitRemoved(unitName, systemdUnitPath);
}
