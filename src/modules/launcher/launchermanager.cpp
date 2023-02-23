// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "launchermanager.h"
#include "launcher.h"
#include "dbusadaptorlauncher.h"

LauncherManager::LauncherManager(QObject *parent)
 : QObject(parent)
 , launcher(new Launcher(this))
{
    qInfo() << "LauncherManager";
    new DBusAdaptorLauncher(launcher);
    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.registerService(dbusService))
    {
        qInfo() << "register service Launcher1 error:" << con.lastError().message();
    }

    if (!con.registerObject(dbusPath, launcher))
    {
        qInfo() << "register object Launcher1 error:" << con.lastError().message();
    }

}

LauncherManager::~LauncherManager()
{

}
