/*
 * Copyright (C) 2022 ~ 2023 Deepin Technology Co., Ltd.
 *
 * Author:     weizhixiang <weizhixiang@uniontech.com>
 *
 * Maintainer: weizhixiang <weizhixiang@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "launchermanager.h"
#include "launcher.h"
#include "dbusadaptorlauncher.h"

LauncherManager::LauncherManager(QObject *parent)
 : QObject(parent)
 , launcher(new Launcher(this))
{
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
