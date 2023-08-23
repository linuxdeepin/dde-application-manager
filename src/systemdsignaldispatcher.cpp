// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "systemdsignaldispatcher.h"

bool SystemdSignalDispatcher::connectToSignals() noexcept
{
    auto &con = ApplicationManager1DBus::instance().globalDestBus();

    if (!con.connect(SystemdService,
                     SystemdObjectPath,
                     SystemdInterfaceName,
                     "UnitNew",
                     this,
                     SLOT(onUnitNew(QString, QDBusObjectPath)))) {
        qCritical() << "can't connect to UnitNew signal of systemd service.";
        return false;
    }

    if (!con.connect(SystemdService,
                     SystemdObjectPath,
                     SystemdInterfaceName,
                     "UnitRemoved",
                     this,
                     SLOT(onUnitRemoved(QString, QDBusObjectPath)))) {
        qCritical() << "can't connect to UnitRemoved signal of systemd service.";
        return false;
    }

    return true;
}

void SystemdSignalDispatcher::onUnitNew(QString unitName, QDBusObjectPath systemdUnitPath)
{
    decltype(auto) appPrefix = u8"app-";
    if (!unitName.startsWith(appPrefix)) {
        return;
    }

    emit SystemdUnitNew(unitName.sliced(sizeof(appPrefix) - 1), systemdUnitPath);
}

void SystemdSignalDispatcher::onUnitRemoved(QString unitName, QDBusObjectPath systemdUnitPath)
{
    decltype(auto) appPrefix = u8"app-";
    if (!unitName.startsWith(appPrefix)) {
        return;
    }

    emit SystemdUnitRemoved(unitName.sliced(sizeof(appPrefix) - 1), systemdUnitPath);
}
