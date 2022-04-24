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

#ifndef DBUSHANDLER_H
#define DBUSHANDLER_H

#include "dbuslauncher.h"
#include "dbuslauncherfront.h"
#include "dbuswm.h"
#include "dbuswmswitcher.h"
#include "dbuskwaylandwindowmanager.h"
#include "windowinfok.h"
#include "dbusplasmawindow.h"
#include "dbusbamfmatcher.h"

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
    bool wlShowingDesktop();
    uint wlActiveWindow();

    /************************* daemon.Launcher1 ***************************/
    void handleLauncherItemChanged(const QString &status, LauncherItemInfo itemInfo, qlonglong categoryID);

    /************************* WMSwitcher ***************************/
    QString getCurrentWM();

    /************************* StartManager ***************************/
    void launchApp(uint32_t timestamp, QStringList files);
    void launchAppAction(uint32_t timestamp, QString file,  QString section);

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
    void handleWlActiveWindowchange();

private:
    Dock *dock;
    QDBusConnection session;

    LauncherBackEnd *launcherEnd;
    LauncherFront *launcherFront;
    com::deepin::WM *wm;
    com::deepin::WMSwitcher *wmSwitcher;
    com::deepin::daemon::kwayland::WindowManager *kwaylandManager;
    org::ayatana::bamf::BamfMatcher *bamfMatcher;
};

#endif // DBUSHANDLER_H
