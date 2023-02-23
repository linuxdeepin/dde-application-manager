// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
        connect(launcher, &Launcher::fullScreenChanged, this, &DBusAdaptorLauncher::FullscreenChanged);
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
    parent()->initItems();
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

void DBusAdaptorLauncher::RequestUninstall(const QString &desktop, bool unused)
{
    Q_UNUSED(unused);

    parent()->requestUninstall(desktop);
}

void DBusAdaptorLauncher::SetDisableScaling(const QString &id, bool value)
{
    parent()->setDisableScaling(id, value);
}

void DBusAdaptorLauncher::SetUseProxy(const QString &id, bool value)
{
    parent()->setUseProxy(id, value);
}
