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

#ifndef DOCK_H
#define DOCK_H

#include "synmodule.h"
#include "docksettings.h"
#include "entries.h"
#include "dbusplasmawindow.h"
#include "dockrect.h"

#include <QStringList>
#include <QTimer>
#include <QMutex>

class WindowIdentify;
class DBusHandler;
class WaylandManager;
class X11Manager;
class WindowInfoK;
class WindowInfoX;

enum class HideState
{
    Unknown,
    Show,
    Hide,
};

// 任务栏
class Dock : public SynModule
{
    Q_OBJECT
public:
    explicit Dock(QObject *parent = nullptr);
    ~Dock();

    bool dockEntry(Entry *entry);
    void undockEntry(Entry *entry);
    QString allocEntryId();
    bool shouldShowOnDock(WindowInfoBase *info);
    void setDdeLauncherVisible(bool visible);
    QString getWMName();
    void setWMName(QString name);
    void setPropHideState(HideState state);
    void attachWindow(WindowInfoBase *info);
    void detachWindow(WindowInfoBase *info);
    void launchApp(const QString desktopFile, uint32_t timestamp, QStringList files);
    void launchAppAction(const QString desktopFile, QString action, uint32_t timestamp);
    bool is3DWM();
    bool isWaylandEnv();
    WindowInfoK *handleActiveWindowChangedK(uint activeWin);
    void handleActiveWindowChanged(WindowInfoBase *info);
    void saveDockedApps();
    void removeAppEntry(Entry *entry);
    void handleWindowGeometryChanged();
    Entry *getEntryByWindowId(XWindow windowId);
    QString getDesktopFromWindowByBamf(XWindow windowId);

    void registerWindowWayland(const QString &objPath);
    void unRegisterWindowWayland(const QString &objPath);
    bool isShowingDesktop();

    AppInfo *identifyWindow(WindowInfoBase *winInfo, QString &innerId);
    void markAppLaunched(AppInfo *appInfo);
    void deleteWindow(XWindow xid);

    ForceQuitAppMode getForceQuitAppStatus();
    QVector<QString> getWinIconPreferredApps();
    void handleLauncherItemDeleted(QString itemPath);
    void handleLauncherItemUpdated(QString itemPath);

    double getOpacity();
    QRect getFrontendWindowRect();
    int getDisplayMode();
    void setDisplayMode(int mode);
    QStringList getDockedApps();
    QStringList getEntryPaths();
    HideMode getHideMode();
    void setHideMode(HideMode mode);
    HideState getHideState();
    void setHideState(HideState state);
    uint getHideTimeout();
    void setHideTimeout(uint timeout);
    uint getIconSize();
    void setIconSize(uint size);
    int getPosition();
    void setPosition(int position);
    uint getShowTimeout();
    void setShowTimeout(uint timeout);
    uint getWindowSizeEfficient();
    void setWindowSizeEfficient(uint size);
    uint getWindowSizeFashion();
    void setWindowSizeFashion(uint size);

    // 设置配置
    void setSynConfig(QByteArray ba);
    QByteArray getSyncConfig();

    /******************************** dbus handler ****************************/
    PlasmaWindow *createPlasmaWindow(QString objPath);
    void listenKWindowSignals(WindowInfoK *windowInfo);
    void removePlasmaWindowHandler(PlasmaWindow *window);
    void presentWindows(QList<uint> windows);

    HideMode getDockHideMode();
    bool isActiveWindow(const WindowInfoBase *win);
    WindowInfoBase *getActiveWindow();
    void doActiveWindow(XWindow xid);
    QList<XWindow> getClientList();
    void setClientList(QList<XWindow> value);

    void closeWindow(XWindow windowId);
    QStringList getEntryIDs();
    void setFrontendWindowRect(int32_t x, int32_t y, uint width, uint height);
    bool isDocked(const QString desktopFile);
    bool requestDock(QString desktopFile, int index);
    bool requestUndock(QString desktopFile);
    void moveEntry(int oldIndex, int newIndex);
    bool isOnDock(QString desktopFile);
    QString queryWindowIdentifyMethod(XWindow windowId);
    QStringList getDockedAppsDesktopFiles();
    QString getPluginSettings();
    void setPluginSettings(QString jsonStr);
    void mergePluginSettings(QString jsonStr);
    void removePluginSettings(QString pluginName, QStringList settingkeys);

    void updateHideState(bool delay);

Q_SIGNALS:
    void serviceRestarted();
    void entryAdded(QDBusObjectPath entryObjPath, int index);
    void entryRemoved(QString id);
    void hideStateChanged(int);
    void frontendWindowRectChanged();

public Q_SLOTS:
    void smartHideModeTimerExpired();
    void attachOrDetachWindow(WindowInfoBase *info);

private:
    void initSettings();
    void updateMenu();
    void initEntries();
    void initDockedApps();
    void initClientList();
    WindowInfoX *findWindowByXidX(XWindow xid);
    WindowInfoK *findWindowByXidK(XWindow xid);
    bool isWindowDockOverlapX(XWindow xid);
    bool hasInterSectionX(const Geometry &windowRect, QRect dockRect);
    bool isWindowDockOverlapK(WindowInfoBase *info);
    bool hasInterSectionK(const DockRect &windowRect, QRect dockRect);
    Entry *getDockedEntryByDesktopFile(const QString &desktopFile);
    bool shouldHideOnSmartHideMode();
    QVector<XWindow> getActiveWinGroup(XWindow xid);

    WindowIdentify *windowIdentify; // 窗口识别
    Entries *entries;   // 所有应用实例
    int entriesSum; // 累计打开的应用数量
    bool ddeLauncherVisible;    // 前端启动器是否可见
    QString wmName; // 窗管名称
    WaylandManager *waylandManager; // wayland窗口管理
    X11Manager *x11Manager;     // X11窗口管理
    QList<XWindow> clientList; // 所有窗口
    QRect frontendWindowRect;    // 前端任务栏大小, 用于智能隐藏时判断窗口是否重合
    HideState hideState;    // 记录任务栏隐藏状态
    QTimer *smartHideTimer; // 任务栏智能隐藏定时器
    WindowInfoBase *activeWindow;// 记录当前活跃窗口信息
    WindowInfoBase *activeWindowOld;// 记录前一个活跃窗口信息
    bool isWayland; // 判断是否为wayland环境
    ForceQuitAppMode forceQuitAppStatus; // 强制退出应用状态
    DBusHandler *dbusHandler;   // 处理dbus交互
    QMutex windowOperateMutex;  // 窗口合并或拆分锁
};

#endif // DOCK_H
