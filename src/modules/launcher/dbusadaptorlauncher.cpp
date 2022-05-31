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

#include "dbusadaptorlauncher.h"

DBusAdaptorLauncher::DBusAdaptorLauncher(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);

    Launcher *launcher = static_cast<Launcher *>(QObject::parent());
    if (launcher) {
        connect(launcher, &Launcher::itemChanged, this, &DBusAdaptorLauncher::ItemChanged);
        connect(launcher, &Launcher::newAppLaunched, this, &DBusAdaptorLauncher::NewAppLaunched);
        connect(launcher, &Launcher::uninstallFailed, this, &DBusAdaptorLauncher::UninstallFailed);
        connect(launcher, &Launcher::uninstallSuccess, this, &DBusAdaptorLauncher::UninstallSuccess);
        connect(launcher, &Launcher::displayModeChanged, this, &DBusAdaptorLauncher::DisplayModeChanged);
        connect(launcher, &Launcher::fullScreenChanged, this, &DBusAdaptorLauncher::FullScreenChanged);
    }
}

DBusAdaptorLauncher::~DBusAdaptorLauncher()
{

}

Launcher *DBusAdaptorLauncher::parent() const
{
    return static_cast<Launcher *>(QObject::parent());
}

int DBusAdaptorLauncher::displayMode() const
{
    return parent()->getDisplayMode();
}

void DBusAdaptorLauncher::setDisplayMode(int value)
{
    parent()->setDisplayMode(value);
}

bool DBusAdaptorLauncher::fullscreen() const
{
    return parent()->getFullScreen();
}

void DBusAdaptorLauncher::setFullscreen(bool value)
{
    parent()->setFullscreen(value);
}

LauncherItemInfoList DBusAdaptorLauncher::GetAllItemInfos()
{
    return parent()->getAllItemInfos();
}

QStringList DBusAdaptorLauncher::GetAllNewInstalledApps()
{
    return parent()->getAllNewInstalledApps();
}

bool DBusAdaptorLauncher::GetDisableScaling(const QString &id)
{
    return parent()->getDisableScaling(id);
}

LauncherItemInfo DBusAdaptorLauncher::GetItemInfo(const QString &id)
{
    return parent()->getItemInfo(id);
}

bool DBusAdaptorLauncher::GetUseProxy(const QString &id)
{
    return parent()->getUseProxy(id);
}

bool DBusAdaptorLauncher::IsItemOnDesktop(const QString &id)
{
    return parent()->isItemOnDesktop(id);
}

bool DBusAdaptorLauncher::RequestRemoveFromDesktop(const QString &id)
{
    return parent()->requestRemoveFromDesktop(id);
}

bool DBusAdaptorLauncher::RequestSendToDesktop(const QString &id)
{
    return parent()->requestSendToDesktop(id);
}

void DBusAdaptorLauncher::RequestUninstall(const QString &id)
{
    parent()->requestUninstall(id);
}

void DBusAdaptorLauncher::SetDisableScaling(const QString &id, bool value)
{
    parent()->setDisableScaling(id, value);
}

void DBusAdaptorLauncher::SetUseProxy(const QString &id, bool value)
{
    parent()->setUseProxy(id, value);
}

