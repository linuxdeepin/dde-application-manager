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

#include "entry.h"
#include "dock.h"
#include "processinfo.h"
#include "dbusadaptorentry.h"
#include "xcbutils.h"

#include <QDebug>
#include <QDBusInterface>

#include <signal.h>

#define XCB XCBUtils::instance()

Entry::Entry(Dock *_dock, AppInfo *_app, QString _innerId, QObject *parent)
    : QObject(parent)
    , dock(_dock)
    , app(nullptr)
    , menu(nullptr)
    , isActive(false)
    , isDocked(false)
    , innerId(_innerId)
    , current(nullptr)
    , currentWindow(0)
    , winIconPreferred(false)
{
    setApp(_app);
    id = dock->allocEntryId();
    name = getName();
    icon = getIcon();
}

Entry::~Entry()
{
    for (auto winInfo : windowInfos) {
        if (winInfo)
            delete winInfo;
    }
    windowInfos.clear();

    if (app) {
        delete app;
        app = nullptr;
    }

    if (menu) {
        delete menu;
        menu = nullptr;
    }
}

bool Entry::isValid()
{
    bool valid = false;
    if (app)
        valid = app->isValidApp();

    return valid;
}

QString Entry::getId()
{
    return id;
}

QString Entry::getName()
{
    QString ret;
    if (app)
        ret = app->getName();

    if (ret.isEmpty() && !!current)
        ret = current->getDisplayName();

    return ret;
}

void Entry::updateName()
{
    setPropName(getName());
}

QString Entry::getIcon()
{
    QString ret;
    if (hasWindow()) {
        if (!current)
            return ret;

        // has window && current not nullptr
        if (winIconPreferred) {
            // try current window icon first
            ret = current->getIcon();
            if (ret.size() > 0)
                return ret;
        }

        if (app) {
            icon = app->getIcon();
            if (icon.size() > 0)
                return icon;
        }
        return current->getIcon();
    }

    if (app) {
        // no window
        return app->getIcon();
    }

    return ret;
}

QString Entry::getInnerId()
{
    return innerId;
}

void Entry::setInnerId(QString _innerId)
{
    innerId = _innerId;
}

QString Entry::getFileName()
{
    QString fileName;
    if (app)
        fileName = app->getFileName();

    return fileName;
}

AppInfo *Entry::getApp()
{
    return app;
}

void Entry::setApp(AppInfo *appinfo)
{
    if (app == appinfo)
        return;

    if (app)
        delete app;

    app = appinfo;
    if (!appinfo) {
        winIconPreferred = true;
        setPropDesktopFile("");
    } else {
        winIconPreferred = false;
        setPropDesktopFile(appinfo->getFileName());
        QString id = app->getId();
        auto perferredApps = dock->getWinIconPreferredApps();
        if (perferredApps.contains(id)) {
            winIconPreferred = true;
            return;
        }

        if (appinfo->getIcon().size() == 0) {
            winIconPreferred = true;
        }
    }
}

bool Entry::getIsDocked()
{
    return isDocked;
}

void Entry::setIsDocked(bool value)
{
    if (value != isDocked) {
        isDocked = value;
        Q_EMIT isDockedChanged(value);
    }
}

// 导出Dbus服务
void Entry::startExport()
{
    if (getId().isEmpty()) {
        qWarning() << "startExport Entry " << getName() << " id is Empty";
        return;
    }

    new DBusAdaptorEntry(this); // export dbus by Adaptor
    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.registerService(dbusService)) {
        qWarning() << "register service Dock1 error:" << con.lastError().message();
        return;
    }

    objctPath = entryDBusObjPathPrefix + getId();
    if (!con.registerObject(objctPath, this))
    {
        qWarning() << "register object Dock1 error:" << con.lastError().message();
        return;
    }
}

// 停止导出Dbus服务
void Entry::stopExport()
{
    if (getId().isEmpty()) {
        qWarning() << "stopExport Entry " << getName() << " id is Empty";
        return;
    }

    QDBusConnection con = QDBusConnection::sessionBus();
    objctPath.clear();
    con.unregisterObject(entryDBusObjPathPrefix + getId());
}

void Entry::setMenu(AppMenu *_menu)
{
    _menu->setDirtyStatus(true);
    if (menu)
        delete menu;

    menu = _menu;
    Q_EMIT menuChanged(menu->getMenuJsonStr());
}

void Entry::updateMenu()
{
    qInfo() <<"Entry: updateMenu";
    AppMenu *appMenu = new AppMenu();
    appMenu->appendItem(getMenuItemLaunch());

    for (auto &item :getMenuItemDesktopActions())
        appMenu->appendItem(item);

    bool hasWin = hasWindow();
    if (hasWin)
        appMenu->appendItem(getMenuItemAllWindows());

    // menu item dock or undock
    qInfo() << "entry " << id << " docked? " << isDocked;
    if (isDocked)
        appMenu->appendItem(getMenuItemUndock());
    else
        appMenu->appendItem(getMenuItemDock());

    if (hasWindow()) {
        if (dock->getForceQuitAppStatus() != ForceQuitAppMode::Disabled) {
            if (app && app->getIdentifyMethod() == "Andriod")
                appMenu->appendItem(getMenuItemForceQuitAndroid());
            else
                appMenu->appendItem(getMenuItemForceQuit());
        }

        if (getAllowedCloseWindows().size() > 0)
            appMenu->appendItem(getMenuItemCloseAll());
    }
    setMenu(appMenu);
}

void Entry::updateIcon()
{
    setPropIcon(getIcon());
}

void Entry::forceUpdateIcon()
{
    icon = getIcon();
    Q_EMIT iconChanged(icon);
}

void Entry::updateIsActive()
{
    bool isActive = false;
    auto activeWin = dock->getActiveWindow();
    if (activeWin)
        isActive = windowInfos.find(activeWin->getXid()) != windowInfos.end();

    setPropIsActive(isActive);
}

WindowInfoBase *Entry::getWindowInfoByPid(int pid)
{
    WindowInfoBase *ret = nullptr;
    for (const auto &windowInfo : windowInfos) {
        if (windowInfo->getPid() == pid) {
            ret = windowInfo;
            break;
        }
    }

    return ret;
}

WindowInfoBase *Entry::getWindowInfoByWinId(XWindow windowId)
{
    WindowInfoBase *ret = nullptr;
    if (windowInfos.find(windowId) != windowInfos.end()) {
        ret = windowInfos[windowId];
    }

    return ret;
}

void Entry::setPropIsDocked(bool docked)
{
    if (isDocked != docked) {
        isDocked = docked;
        Q_EMIT isDockedChanged(docked);
    }
}

void Entry::setPropIcon(QString value)
{
    if (value != icon) {
        icon = value;
        Q_EMIT iconChanged(value);
    }
}

void Entry::setPropName(QString value)
{
    if (value != name) {
        name = value;
        Q_EMIT nameChanged(value);
    }
}

void Entry::setPropIsActive(bool active)
{
    if (isActive != active) {
        isActive = active;
        Q_EMIT isActiveChanged(active);
    }
}

void Entry::setCurrentWindowInfo(WindowInfoBase *windowInfo)
{
    current = windowInfo;
    setPropCurrentWindow(current ? current->getXid() : 0);
}

void Entry::setPropCurrentWindow(XWindow value)
{
    if (value != currentWindow) {
        currentWindow = value;
        Q_EMIT currentWindowChanged(value);
    }
}

WindowInfoBase *Entry::getCurrentWindowInfo()
{
    return current;
}

/**
 * @brief Entry::findNextLeader
 * @return
 */
WindowInfoBase *Entry::findNextLeader()
{
    auto xids = windowInfos.keys();
    qSort(xids);
    XWindow curWinId = current->getXid();
    int index = xids.indexOf(curWinId);
    if (index < 0)
        return nullptr;

    // 如果当前窗口是最大， 返回xids[0], 否则返回xids[index + 1]
    int nextIndex = 0;
    if (index < xids.size() - 1)
        nextIndex = index + 1;

    return windowInfos[xids[nextIndex]];
}

QString Entry::getExec(bool oneLine)
{
    if (!current)
        return "";

    ProcessInfo *process = current->getProcess();
    if (process) {
        if (oneLine)
            return process->getOneCommandLine().c_str();
        else
            return process->getShellScriptLines().c_str();
    }

    return "";
}

bool Entry::hasWindow()
{
    return windowInfos.size() > 0;
}

/**
 * @brief Entry::updateExportWindowInfos 同步更新导出窗口信息
 */
void Entry::updateExportWindowInfos()
{
    ExportWindowInfoList infos;
    for (auto info : windowInfos) {
        XWindow xid = info->getXid();
        QString title = info->getTitle();
        bool flash = info->isDemandingAttention();
        infos.push_back({xid, title, flash});
    }

    bool changed = false;
    if (infos.size() == exportWindowInfos.size()) {
        sortExprotWindowInfoList(infos);
        sortExprotWindowInfoList(exportWindowInfos);
        for (int i = 0; i < infos.size(); i++) {
            if (infos[i].getXid() != exportWindowInfos[i].getXid()
                    || infos[i].getFlash() != exportWindowInfos[i].getFlash()
                    || infos[i].getTitle() != exportWindowInfos[i].getTitle()) {
                changed = true;
                break;
            }
        }
    } else {
        changed = true;
    }

    if (changed) {
        Q_EMIT windowInfosChanged(infos);
    }

    exportWindowInfos = infos;
}

// 分离窗口， 返回是否需要从任务栏remove
bool Entry::detachWindow(WindowInfoBase *info)
{
    info->setEntry(nullptr);
    XWindow winId = info->getXid();
    deleteWindow(winId);

    if (windowInfos.size() == 0) {
        if (!isDocked)  // 既无窗口也非驻留应用，无需在任务栏显示
            return true;

        setCurrentWindowInfo(nullptr);
    } else {
        for (auto window : windowInfos) {
            if (window) {   // 选择第一个窗口作为当前窗口
                setCurrentWindowInfo(window);
                break;
            }
        }
    }

    updateExportWindowInfos();
    updateIcon();
    updateIsActive();
    updateMenu();

    return false;
}

bool Entry::attachWindow(WindowInfoBase *info)
{
    XWindow winId = info->getXid();
    qInfo() << "attatchWindow: window id:" << winId;
    info->setEntry(this);

    if (windowInfos.find(winId) != windowInfos.end()) {
        qInfo() << "attachWindow: window " << winId << " is already attached";
        return false;
    }

    windowInfos[winId] = info;
    updateExportWindowInfos();
    updateIsActive();

    if (!current) {
        // from no window to has window
        setCurrentWindowInfo(info);
    }

    updateIcon();
    updateMenu();
    return true;
}

void Entry::launchApp(uint32_t timestamp)
{
    dock->launchApp(app->getFileName(), timestamp, QStringList());
}

bool Entry::containsWindow(XWindow xid)
{
    return windowInfos.find(xid) != windowInfos.end();
}

void Entry::deleteWindow(XWindow xid)
{
    WindowInfoBase *info = windowInfos[xid];
    windowInfos.remove(xid);
    for (int i = 0; i < exportWindowInfos.size(); i++) {
        if (exportWindowInfos[i].getXid() == xid) {
            exportWindowInfos.removeAt(i);
            break;
        }
    }

    if (info) {
        delete info;
    }
}

// 处理菜单项
void Entry::handleMenuItem(uint32_t timestamp, QString itemId)
{
    menu->handleAction(timestamp, itemId);
}

// 处理拖拽事件
void Entry::handleDragDrop(uint32_t timestamp, QStringList files)
{
    dock->launchApp(app->getFileName(), timestamp, files);
}

// 驻留
void Entry::requestDock()
{
    if (dock->dockEntry(this)) {
        dock->saveDockedApps();
    }
}

// 取消驻留
void Entry::requestUndock()
{
    dock->undockEntry(this);
}

void Entry::newInstance(uint32_t timestamp)
{
    QStringList files;
    dock->launchApp(app->getFileName(), timestamp, files);
}

// 检查应用窗口分离、合并状态
void Entry::check()
{
    for (auto iter = windowInfos.begin(); iter != windowInfos.end(); iter++) {
        dock->attachOrDetachWindow(iter.value());
    }
}

// 强制退出
void Entry::forceQuit()
{
    QMap<int, QVector<WindowInfoBase*>> pidWinInfoMap;
    for (auto iter = windowInfos.begin(); iter != windowInfos.end(); iter++) {
        int pid = iter.value()->getPid();
        if (pid != 0) {
            pidWinInfoMap[pid].push_back(iter.value());
        } else {
            iter.value()->killClient();
        }

        for (auto iter = pidWinInfoMap.begin(); iter != pidWinInfoMap.end(); iter++) {
            if (!killProcess(iter.key())) {         // kill pid
                for (auto &info : iter.value()) {   // kill window
                    info->killClient();
                }
            }
        }
    }
}

void Entry::presentWindows()
{
    QList<uint> windows;
    for (auto iter = windowInfos.begin(); iter != windowInfos.end(); iter++)
        windows.push_back(iter.key());

    dock->presentWindows(windows);
}

/**
 * @brief Entry::active 激活窗口
 * @param timestamp
 */
void Entry::active(uint32_t timestamp)
{
    if (dock->getHideMode() == HideMode::SmartHide) {
        dock->setPropHideState(HideState::Show);
        dock->updateHideState(true);
    }

    // 无窗口则直接启动
    if (!hasWindow()) {
        launchApp(timestamp);
        return;
    }

    if (!current) {
        qWarning() << "active: current window is nullptr";
        return;
    }

    WindowInfoBase *winInfo = current;
    if (dock->isWaylandEnv()) {
        // wayland环境
        if (!dock->isActiveWindow(winInfo)) {
            winInfo->activate();
        } else {
            bool showing = dock->isShowingDesktop();
            if (showing || winInfo->isMinimized()) {
                winInfo->activate();
            } else {
                if (windowInfos.size() == 1) {
                    winInfo->minimize();
                } else {
                    WindowInfoBase *nextWin = findNextLeader();
                    if (nextWin) {
                        nextWin->activate();
                    }
                }
            }
        }
    } else {
        // X11环境
        XWindow xid = winInfo->getXid();
        WindowInfoBase *activeWin = dock->getActiveWindow();
        if (xid != activeWin->getXid()) {
            dock->doActiveWindow(xid);
        } else {
            bool found = false;
            bool hiddenAtom = XCB->getAtom("_NET_WM_STATE_HIDDEN");
            for (auto state : XCB->getWMState(xid)) {
                if (hiddenAtom == state) {
                    found = true;
                    break;
                }
            }

            if (found) {
                // 激活隐藏窗口
                dock->doActiveWindow(xid);
            } else {
                if (windowInfos.size() == 1) {
                    XCB->minimizeWindow(xid);
                } else if (dock->getActiveWindow()->getXid() == xid) {
                    WindowInfoBase *nextWin = findNextLeader();
                    if (nextWin) {
                        nextWin->activate();
                    }
                }
            }
        }
    }
}

XWindow Entry::getCurrentWindow()
{
    return currentWindow;
}

QString Entry::getDesktopFile()
{
    return desktopFile;
}

bool Entry::getIsActive()
{
    return isActive;
}

QString Entry::getMenu()
{
    return menu->getMenuJsonStr();
}

QVector<XWindow> Entry::getAllowedClosedWindowIds()
{
    QVector<XWindow> ret;
    for (auto iter = windowInfos.begin(); iter != windowInfos.end(); iter++) {
        WindowInfoBase *info = iter.value();
        if (info && info->allowClose())
            ret.push_back(iter.key());
    }

    return ret;
}

ExportWindowInfoList Entry::getExportWindowInfos()
{
    return exportWindowInfos;
}

QVector<WindowInfoBase *> Entry::getAllowedCloseWindows()
{
    QVector<WindowInfoBase *> ret;
    for (auto iter = windowInfos.begin(); iter != windowInfos.end(); iter++) {
        WindowInfoBase *info = iter.value();
        if (info && info->allowClose())
            ret.push_back(info);
    }

    return ret;
}

QVector<AppMenuItem> Entry::getMenuItemDesktopActions()
{
    QVector<AppMenuItem> ret;
    if (!app)
        return ret;

    for (auto action : app->getActions()) {
        AppMenuAction fn = [=](uint32_t timestamp) {
            qInfo() << "do MenuItem: " << action.name.c_str();
            dock->launchAppAction(app->getFileName(), action.section.c_str(), timestamp);
        };

        AppMenuItem item;
        item.text = action.name.c_str();
        item.action = fn;
        item.isActive = true;
        ret.push_back(item);
    }

    return ret;
}

AppMenuItem Entry::getMenuItemLaunch()
{
    QString itemName;
    if (hasWindow())
        itemName = getName();
    else
        itemName = "Open";

    AppMenuAction  fn = [&](uint32_t timestamp) {
        qInfo() << "do MenuItem: Open";
        this->launchApp(timestamp);
    };

    AppMenuItem item;
    item.text = itemName;
    item.action = fn;
    item.isActive = true;
    return item;
}

AppMenuItem Entry::getMenuItemCloseAll()
{
    AppMenuAction fn = [&](uint32_t timestamp) {
        qInfo() << "do MenuItem: Close All";
        auto winInfos = getAllowedCloseWindows();

        // 从大到小排序， 方便后面关闭窗口
        for (int i = 0; i < winInfos.size() - 1; i++) {
            for (int j = i + 1; j < winInfos.size(); j++) {
                if (winInfos[i]->getCreatedTime() < winInfos[j]->getCreatedTime()) {
                    auto info = winInfos[i];
                    winInfos[i] = winInfos[j];
                    winInfos[j] = info;
                }
            }
        }

        for (auto info : winInfos) {
            qInfo() << "close WindowId " << info->getXid();
            info->close(timestamp);
        }
    };

    AppMenuItem item;
    item.text = "Close All";
    item.action = fn;
    item.isActive = true;
    return item;
}

AppMenuItem Entry::getMenuItemForceQuit()
{
    bool active = dock->getForceQuitAppStatus() != ForceQuitAppMode::Deactivated;
    AppMenuAction fn = [&](uint32_t) {
        qInfo() << "do MenuItem: Force Quit";
        forceQuit();
    };

    AppMenuItem item;
    item.text = "Force Quit";
    item.action = fn;
    item.isActive = active;
    return item;
}

//dock栏上Android程序的Force Quit功能
AppMenuItem Entry::getMenuItemForceQuitAndroid()
{
    bool active = dock->getForceQuitAppStatus() != ForceQuitAppMode::Deactivated;
    auto allowedCloseWindows = getAllowedCloseWindows();
    AppMenuAction fn = [](uint32_t){};
    if (allowedCloseWindows.size() > 0) {
        qInfo() << "do MenuItem: Force Quit";
        AppMenuAction fn = [&](uint32_t timestamp) {
            for (auto info : allowedCloseWindows) {
                info->close(timestamp);
            }
        };
    }

    AppMenuItem item;
    item.text = "Force Quit";
    item.action = fn;
    item.isActive = active;
    return item;
}

AppMenuItem Entry::getMenuItemDock()
{
    AppMenuItem item;
    item.text = "Dock";
    item.action = [&](uint32_t) {
        qInfo() << "do MenuItem: Dock";
        requestDock();
    };
    item.isActive = true;
    return item;
}

AppMenuItem Entry::getMenuItemUndock()
{
    AppMenuItem item;
    item.text = "Undock";
    item.action = [&](uint32_t) {
        qInfo() << "do MenuItem: Undock";
        requestUndock();
    };
    item.isActive = true;
    return item;
}

AppMenuItem Entry::getMenuItemAllWindows()
{
    AppMenuItem item;
    item.text = "All Windows";
    item.action = [&](uint32_t) {
        qInfo() << "do MenuItem: All Windows";
        presentWindows();
    };
    item.isActive = true;
    item.hint = menuItemHintShowAllWindows;
    return item;
}

bool Entry::killProcess(int pid)
{
    bool ret = true;
    Process p(pid);
    if (p.isExist()) {
        ret = !kill(pid, SIGTERM);
    }

    return ret;
}

bool Entry::setPropDesktopFile(QString value)
{
    bool ret = false;
    if (value != desktopFile) {
        desktopFile = value;
        ret = true;
        Q_EMIT desktopFileChanged(value);
    }
    return ret;
}



