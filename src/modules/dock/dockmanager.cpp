/*
 * Copyright (C) 2021 ~ 2022 Deepin Technology Co., Ltd.
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

#include "dockmanager.h"
#include "dock.h"

#include <QDBusConnection>
#include <QDebug>
#include <QDBusError>

DockManager::DockManager(QObject *parent)
 : QObject(parent)
 , dock(new Dock(this))
{
    adaptor = new DBusAdaptorDock(dock);
    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.registerService(dbusService)) {
        qWarning() << "register service Dock1 error:" << con.lastError().message();
        return;
    }

    if (!con.registerObject(dbusPath, dock, QDBusConnection::ExportAdaptors))
    {
        qWarning() << "register object Dock1 error:" << con.lastError().message();
        return;
    }


}

DockManager::~DockManager()
{

}
