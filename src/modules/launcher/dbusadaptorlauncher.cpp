#include "dbusadaptorlauncher.h"

DBusAdaptorLauncher::DBusAdaptorLauncher(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
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
    // destructor
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

bool DBusAdaptorLauncher::AddAutostart(const QString &desktopFile)
{
    QDBusInterface interface = QDBusInterface("com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager");
    QDBusReply<bool> reply = interface.call("AddAutostart", desktopFile);
    return reply.isValid() ? reply.value() : false;
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

bool DBusAdaptorLauncher::LaunchWithTimestamp(const QString &desktopFile, int time)
{

    QDBusInterface interface = QDBusInterface("com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager");
    QDBusReply<bool> reply = interface.call("LaunchWithTimestamp", desktopFile, time);
    return reply.isValid() ? reply.value() : false;
}

bool DBusAdaptorLauncher::RemoveAutostart(const QString &desktopFile)
{
    QDBusInterface interface = QDBusInterface("com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager");
    QDBusReply<bool> reply = interface.call("RemoveAutostart", desktopFile);
    return reply.isValid() ? reply.value() : false;
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

