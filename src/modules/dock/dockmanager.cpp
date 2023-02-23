// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
