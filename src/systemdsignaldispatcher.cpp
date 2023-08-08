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

void SystemdSignalDispatcher::onUnitNew(QString serviceName, QDBusObjectPath systemdUnitPath)
{
    if (!serviceName.startsWith("app-")) {
        return;
    }

    emit SystemdUnitNew(serviceName.sliced(4), systemdUnitPath);  // remove "app-"
}

void SystemdSignalDispatcher::onUnitRemoved(QString serviceName, QDBusObjectPath systemdUnitPath)
{
    if (!serviceName.startsWith("app-")) {
        return;
    }

    emit SystemdUnitRemoved(serviceName.sliced(4), systemdUnitPath);
}
