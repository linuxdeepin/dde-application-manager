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
#include "impl/application_manager.h"

#include <QDBusConnection>
#include <QDebug>
#include <QDBusError>

DockManager::DockManager(ApplicationManager *parent)
 : QObject(parent)
 , m_dock(new Dock(parent, this))
{
    qInfo() << "DockManager";
    m_adaptor = new DBusAdaptorDock(m_dock);
    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.registerService(dbusService)) {
        qWarning() << "register service Dock1 error:" << con.lastError().message();
        return;
    }

    if (!con.registerObject(dbusPath, m_dock, QDBusConnection::ExportAdaptors)) {
        qWarning() << "register object Dock1 error:" << con.lastError().message();
    }

    m_dock->serviceRestarted();
}

DockManager::~DockManager()
{

}
