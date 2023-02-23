// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBUSHANDLER_H
#define DBUSHANDLER_H

#include "dbuslauncher.h"
#include "dbuslauncherfront.h"
#include "dbuswm.h"
#include "dbuswmswitcher.h"
#include "dbuskwaylandwindowmanager.h"
#include "windowinfok.h"
#include "dbusplasmawindow.h"
#include "dbusxeventmonitor.h"

#include <QObject>
#include <QDBusConnection>
#include <QDBusMessage>

class Dock;

// 处理DBus交互
class DBusHandler : public QObject
{
    Q_OBJECT
public:
    explicit DBusHandler(Dock *_dock, QObject *parent = nullptr);

    /************************* KWayland/WindowManager ***************************/
    void listenWaylandWMSignals();
    void loadClientList();
    bool wlShowingDesktop();
    uint wlActiveWindow();

    /************************* daemon.Launcher1 ***************************/
    void handleLauncherItemChanged(const QString &status, LauncherItemInfo itemInfo, qlonglong categoryID);

    /************************* WMSwitcher ***************************/
    QString getCurrentWM();

    /************************* StartManager ***************************/
    void launchApp(QString desktopFile, uint32_t timestamp, QStringList files);
    void launchAppAction(QString desktopFile, QString action, uint32_t timestamp);

    /************************* AlRecorder1 ***************************/
    void markAppLaunched(const QString &filePath);

    /************************* KWayland.PlasmaWindow ***************************/
    void listenKWindowSignals(WindowInfoK *windowInfo);
    PlasmaWindow *createPlasmaWindow(QString objPath);
    void removePlasmaWindowHandler(PlasmaWindow *window);

    /************************* WM ***************************/
    void presentWindows(QList<uint> windows);

    /************************* bamf ***************************/
    // XWindow -> desktopFile
    QString getDesktopFromWindowByBamf(XWindow windowId);

private Q_SLOTS:
    void handleWlActiveWindowChange();
    void onActiveWindowButtonRelease(int type, int x, int y, const QString &key);

private:
    Dock *m_dock;
    QDBusConnection m_session;

    LauncherBackEnd *m_launcherEnd;
    LauncherFront *m_launcherFront;
    com::deepin::WM *m_wm;
    org::deepin::dde::WMSwitcher1 *m_wmSwitcher;
    org::deepin::dde::kwayland1::WindowManager *m_kwaylandManager;

    org::deepin::dde::XEventMonitor1 *m_xEventMonitor;
    QString m_activeWindowMonitorKey;
};

#endif // DBUSHANDLER_H
