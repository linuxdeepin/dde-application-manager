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
    , m_current(nullptr)
    , m_currentWindow(0)
    , m_winIconPreferred(false)
    , m_mode(getCurrentMode())
{
    setApp(_app);
    id = dock->allocEntryId();
    name = getName();
    icon = getIcon();
}

Entry::~Entry()
{
    for (auto winInfo : m_windowInfoMap) {
        if (winInfo)
            delete winInfo;
    }
    m_windowInfoMap.clear();

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
    return app ? app->isValidApp() : false;
}

QString Entry::getId() const
{
    return id;
}

QString Entry::path() const
{
    return entryDBusObjPathPrefix + getId();
}

QString Entry::getName()
{
    QString ret;
    if (app) {
        ret = app->getName();
    }

    if (ret.isEmpty() && m_current) {
        ret = m_current->getDisplayName();
    }

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
        if (!m_current) {
            return ret;
        }

        // has window && current not nullptr
        if (m_winIconPreferred) {
            // try current window icon first
            ret = m_current->getIcon();
            if (ret.size() > 0) {
                return ret;
            }
        }

        if (app) {
            icon = app->getIcon();
            if (icon.size() > 0) {
                return icon;
            }
        }

        return m_current->getIcon();
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
    return app ? app->getFileName() : QString();
}

AppInfo *Entry::getApp()
{
    return app;
}

void Entry::setApp(AppInfo *appinfo)
{
    if (app == appinfo) {
        return;
    }

    if (app) {
        delete app;
    }

    app = appinfo;
    if (!appinfo) {
        m_winIconPreferred = true;
        setPropDesktopFile("");
    } else {
        m_winIconPreferred = false;
        setPropDesktopFile(appinfo->getFileName());
        QString id = app->getId();
        auto perferredApps = dock->getWinIconPreferredApps();
        if (perferredApps.contains(id)) {
            m_winIconPreferred = true;
            return;
        }

        if (appinfo->getIcon().size() == 0) {
            m_winIconPreferred = true;
        }
    }
}

bool Entry::getIsDocked() const
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

// 导出DBus服务
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

    if (!con.registerObject(path(), this)) {
        qWarning() << "register object Dock1 error:" << con.lastError().message();
    }
}

// 停止导出DBus服务
void Entry::stopExport()
{
    if (getId().isEmpty()) {
        qWarning() << "stopExport Entry " << getName() << " id is Empty";
        return;
    }

    QDBusConnection con = QDBusConnection::sessionBus();
    con.unregisterObject(path());
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

    if (hasWindow())
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

int Entry::getCurrentMode()
{
    // 只要当前应用是已经驻留的应用，则让其显示为Normal
    if (getIsDocked())
        return ENTRY_NORMAL;

    // 对于未驻留的应用则做如下处理
    if (static_cast<DisplayMode>(dock->getDisplayMode()) == DisplayMode::Efficient) {
        // 高效模式下，只有存在子窗口的，则让其为nornal，没有子窗口的，一般不让其显示
        return hasWindow() ? ENTRY_NORMAL : ENTRY_NONE;
    }
    // 时尚模式下对未驻留应用做如下处理
    // 如果开启了最近打开应用的功能，则显示到最近打开区域（ENTRY_RECENT）
    if (dock->showRecent())
        return ENTRY_RECENT;

    // 未开启最近使用应用的功能，如果有子窗口，则显示成通用的(ENTRY_NORMAL)，如果没有子窗口，则不显示(ENTRY_NONE)
    return hasWindow() ? ENTRY_NORMAL : ENTRY_NONE;
}

void Entry::updateMode()
{
    int currentMode = getCurrentMode();
    if (m_mode != currentMode) {
        m_mode = currentMode;
        Q_EMIT modeChanged(m_mode);
    }
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
    if (activeWin) {
        // 判断活跃窗口是否属于当前应用
        isActive = m_windowInfoMap.find(activeWin->getXid()) != m_windowInfoMap.end();
    }

    setPropIsActive(isActive);
}

WindowInfoBase *Entry::getWindowInfoByPid(int pid)
{
    for (const auto &windowInfo : m_windowInfoMap) {
        if (windowInfo->getPid() == pid)
            return windowInfo;
    }

    return nullptr;
}

WindowInfoBase *Entry::getWindowInfoByWinId(XWindow windowId)
{
    if (m_windowInfoMap.find(windowId) != m_windowInfoMap.end())
        return m_windowInfoMap[windowId];

    return nullptr;
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
    m_current = windowInfo;
    setPropCurrentWindow(m_current ? m_current->getXid() : 0);
}

void Entry::setPropCurrentWindow(XWindow value)
{
    if (value != m_currentWindow) {
        m_currentWindow = value;
        Q_EMIT currentWindowChanged(value);
    }
}

WindowInfoBase *Entry::getCurrentWindowInfo()
{
    return m_current;
}

/**
 * @brief Entry::findNextLeader
 * @return
 */
WindowInfoBase *Entry::findNextLeader()
{
    auto xids = m_windowInfoMap.keys();
    qSort(xids);
    XWindow curWinId = m_current->getXid();
    int index = xids.indexOf(curWinId);
    if (index < 0)
        return nullptr;

    // 如果当前窗口是最大， 返回xids[0], 否则返回xids[index + 1]
    int nextIndex = 0;
    if (index < xids.size() - 1)
        nextIndex = index + 1;

    return m_windowInfoMap[xids[nextIndex]];
}

QString Entry::getExec(bool oneLine)
{
    if (!m_current)
        return "";

    ProcessInfo *process = m_current->getProcess();
    if (process) {
        if (oneLine)
            return process->getOneCommandLine().c_str();

        return process->getShellScriptLines().c_str();
    }

    return "";
}

bool Entry::hasWindow()
{
    return m_windowInfoMap.size() > 0;
}

/**
 * @brief Entry::updateExportWindowInfos 同步更新导出窗口信息
 */
void Entry::updateExportWindowInfos()
{
    WindowInfoMap infos;
    for (auto info : m_windowInfoMap) {
        WindowInfo winInfo;
        XWindow xid = info->getXid();
        winInfo.title = info->getTitle();
        winInfo.attention = info->isDemandingAttention();
        winInfo.uuid = info->uuid();
        infos[xid] = winInfo;
    }

    bool changed = true;
    if (infos.size() == m_exportWindowInfos.size()) {
        changed = false;
        for (auto iter = infos.begin(); iter != infos.end(); iter++) {
            XWindow xid = iter.key();
            if (infos[xid].title != m_exportWindowInfos[xid].title ||
                    infos[xid].attention != m_exportWindowInfos[xid].attention ||
                    infos[xid].uuid != m_exportWindowInfos[xid].uuid) {
                changed = true;
                break;
            }
        }
    }

    if (changed) {
        Q_EMIT windowInfosChanged(infos);
    }

    // 更新导出的窗口信息
    m_exportWindowInfos = infos;
}

// 分离窗口， 返回是否需要从任务栏remove
bool Entry::detachWindow(WindowInfoBase *info)
{
    info->setEntry(nullptr);
    XWindow winId = info->getXid();
    if (m_windowInfoMap.contains(winId)) {
        WindowInfoBase *info = m_windowInfoMap[winId];
        m_windowInfoMap.remove(winId);
        delete info;
    }

    if (m_windowInfoMap.isEmpty()) {
        if (!isDocked) {
            // 既无窗口也非驻留应用，并且不是最近打开，无需在任务栏显示
            return true;
        }

        Q_EMIT windowInfosChanged(WindowInfoMap());
        setCurrentWindowInfo(nullptr);
    } else {
        for (auto window : m_windowInfoMap) {
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

bool Entry::isShowOnDock() const
{
    // 当前应用显示图标的条件是
    // 如果该图标已经固定在任务栏上，则始终显示
    if (getIsDocked())
        return true;

    // 1.时尚模式下，如果开启了显示最近使用，则不管是否有子窗口，都在任务栏上显示
    // 如果没有开启显示最近使用，则只显示有子窗口的
    if (static_cast<DisplayMode>(dock->getDisplayMode()) == DisplayMode::Fashion)
        return (dock->showRecent() || m_exportWindowInfos.size() > 0);

    // 2.高效模式下，只有该应用有打开窗口才显示
    return m_exportWindowInfos.size() > 0;
}

bool Entry::attachWindow(WindowInfoBase *info)
{
    XWindow winId = info->getXid();
    qInfo() << "attatchWindow: window id:" << winId;
    info->setEntry(this);

    if (m_windowInfoMap.find(winId) != m_windowInfoMap.end()) {
        qInfo() << "attachWindow: window " << winId << " is already attached";
        return false;
    }

    bool lastShowOnDock = isShowOnDock();
    m_windowInfoMap[winId] = info;
    updateExportWindowInfos();
    updateIsActive();

    if (!m_current) {
        // from no window to has window
        setCurrentWindowInfo(info);
    }

    updateIcon();
    updateMenu();

    if (!lastShowOnDock && isShowOnDock()) {
        // 新打开的窗口始终显示到最后
        Q_EMIT dock->entryAdded(QDBusObjectPath(path()), -1);
    }

    return true;
}

void Entry::launchApp(uint32_t timestamp)
{
    dock->launchApp(app->getFileName(), timestamp, QStringList());
}

bool Entry::containsWindow(XWindow xid)
{
    return m_windowInfoMap.find(xid) != m_windowInfoMap.end();
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
void Entry::requestDock(bool dockToEnd)
{
    if (dock->dockEntry(this, dockToEnd)) {
        dock->saveDockedApps();
    }
}

// 取消驻留
void Entry::requestUndock(bool dockToEnd)
{
    dock->undockEntry(this, dockToEnd);
}

void Entry::newInstance(uint32_t timestamp)
{
    QStringList files;
    dock->launchApp(app->getFileName(), timestamp, files);
}

// 检查应用窗口分离、合并状态
void Entry::check()
{
    for (auto iter = m_windowInfoMap.begin(); iter != m_windowInfoMap.end(); iter++) {
        dock->attachOrDetachWindow(iter.value());
    }
}

// 强制退出
void Entry::forceQuit()
{
    QMap<int, QVector<WindowInfoBase*>> pidWinInfoMap;
    for (auto iter = m_windowInfoMap.begin(); iter != m_windowInfoMap.end(); iter++) {
        int pid = iter.value()->getPid();
        if (pid != 0) {
            pidWinInfoMap[pid].push_back(iter.value());
        } else {
            iter.value()->killClient();
        }
    }

    for (auto iter = pidWinInfoMap.begin(); iter != pidWinInfoMap.end(); iter++) {
        if (!killProcess(iter.key())) {         // kill pid
            for (auto &info : iter.value()) {   // kill window
                info->killClient();
            }
        }
    }
    // 所有的窗口已经退出后，清空m_windowInfoMap内容
    m_windowInfoMap.clear();
    // 退出所有的进程后，及时更新当前剩余的窗口数量
    updateExportWindowInfos();
}

void Entry::presentWindows()
{
    QList<uint> windows;
    for (auto iter = m_windowInfoMap.begin(); iter != m_windowInfoMap.end(); iter++) {
        windows.push_back(iter.key());
    }

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
        dock->updateHideState(false);
    }

    // 无窗口则直接启动
    if (!hasWindow()) {
        launchApp(timestamp);
        return;
    }

    if (!m_current) {
        qWarning() << "active: current window is nullptr";
        return;
    }

    WindowInfoBase *winInfo = m_current;
    if (dock->isWaylandEnv()) {
        // wayland环境
        if (!dock->isActiveWindow(winInfo)) {
            winInfo->activate();
        } else {
            bool showing = dock->isShowingDesktop();
            if (showing || winInfo->isMinimized()) {
                winInfo->activate();
            } else if (m_windowInfoMap.size() == 1) {
                winInfo->minimize();
            } else {
                WindowInfoBase *nextWin = findNextLeader();
                if (nextWin) {
                    nextWin->activate();
                }
            }
        }
    } else {
        // X11环境
        XWindow xid = winInfo->getXid();
        WindowInfoBase *activeWin = dock->getActiveWindow();
        if (activeWin && xid != activeWin->getXid()) {
            dock->doActiveWindow(xid);
        } else {
            bool found = false;
            XWindow hiddenAtom = XCB->getAtom("_NET_WM_STATE_HIDDEN");
            for (auto state : XCB->getWMState(xid)) {
                if (hiddenAtom == state) {
                    found = true;
                    break;
                }
            }

            if (found) {
                // 激活隐藏窗口
                dock->doActiveWindow(xid);
            } else if (m_windowInfoMap.size() == 1) {
                // 窗口图标化
                XCB->minimizeWindow(xid);
            } else if (dock->getActiveWindow() && dock->getActiveWindow()->getXid() == xid) {
                WindowInfoBase *nextWin = findNextLeader();
                if (nextWin) {
                    nextWin->activate();
                }
            }
        }
    }
}

void Entry::activeWindow(quint32 winId)
{
    if (dock->isWaylandEnv()) {
        if (!m_windowInfoMap.contains(winId))
            return;

        WindowInfoBase *winInfo = m_windowInfoMap[winId];
        if (dock->isActiveWindow(winInfo)) {
            bool showing = dock->isShowingDesktop();
            if (showing || winInfo->isMinimized()) {
                winInfo->activate();
            } else if (m_windowInfoMap.size() == 1) {
                winInfo->minimize();
            } else {
                WindowInfoBase *nextWin = findNextLeader();
                if (nextWin) {
                    nextWin->activate();
                }
            }
        } else {
            winInfo->activate();
        }
    } else {
        dock->doActiveWindow(winId);
    }
}

int Entry::mode()
{
    return m_mode;
}

XWindow Entry::getCurrentWindow()
{
    return m_currentWindow;
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
    for (auto iter = m_windowInfoMap.begin(); iter != m_windowInfoMap.end(); iter++) {
        WindowInfoBase *info = iter.value();
        if (info && info->allowClose())
            ret.push_back(iter.key());
    }

    return ret;
}

WindowInfoMap Entry::getExportWindowInfos()
{
    return m_exportWindowInfos;
}

QVector<WindowInfoBase *> Entry::getAllowedCloseWindows()
{
    QVector<WindowInfoBase *> ret;
    for (auto iter = m_windowInfoMap.begin(); iter != m_windowInfoMap.end(); iter++) {
        WindowInfoBase *info = iter.value();
        if (info && info->allowClose()) {
            ret.push_back(info);
        }
    }

    return ret;
}

QVector<AppMenuItem> Entry::getMenuItemDesktopActions()
{
    QVector<AppMenuItem> ret;
    if (!app) {
        return ret;
    }

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
    if (hasWindow()) {
        itemName = getName();
    } else {
        itemName = tr("Open");
    }

    AppMenuAction fn = [this](uint32_t timestamp) {
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
    AppMenuAction fn = [this](uint32_t timestamp) {
        qInfo() << "do MenuItem: Close All";
        auto winInfos = getAllowedCloseWindows();

        // 根据创建时间从大到小排序， 方便后续关闭窗口
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

        // 关闭窗口后，主动刷新事件
        XCB->flush();
    };

    AppMenuItem item;
    item.text = tr("Close All");
    item.action = fn;
    item.isActive = true;
    return item;
}

AppMenuItem Entry::getMenuItemForceQuit()
{
    bool active = dock->getForceQuitAppStatus() != ForceQuitAppMode::Deactivated;
    AppMenuAction fn = [this](uint32_t) {
        qInfo() << "do MenuItem: Force Quit";
        forceQuit();
    };

    AppMenuItem item;
    item.text = tr("Force Quit");
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
    item.text = tr("Force Quit");
    item.action = fn;
    item.isActive = active;
    return item;
}

AppMenuItem Entry::getMenuItemDock()
{
    AppMenuItem item;
    item.text = tr("Dock");
    item.action = [this](uint32_t) {
        qInfo() << "do MenuItem: Dock";
        requestDock(true);
    };

    item.isActive = true;
    return item;
}

AppMenuItem Entry::getMenuItemUndock()
{
    AppMenuItem item;
    item.text = tr("Undock");
    item.action = [this](uint32_t) {
        qInfo() << "do MenuItem: Undock";
        requestUndock(true);
    };

    item.isActive = true;
    return item;
}

AppMenuItem Entry::getMenuItemAllWindows()
{
    AppMenuItem item;
    item.text = tr("All Windows");
    item.action = [this](uint32_t) {
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
    if (value != desktopFile) {
        desktopFile = value;
        Q_EMIT desktopFileChanged(value);
        return true;
    }

    return false;
}
