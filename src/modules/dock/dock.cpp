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

#include "dock.h"
#include "windowidentify.h"
#include "windowinfok.h"
#include "xcbutils.h"
#include "dbushandler.h"
#include "appinfo.h"
#include "waylandmanager.h"
#include "x11manager.h"
#include "windowinfobase.h"
#include "dbusplasmawindow.h"

#include "macro.h"

#include <QDir>
#include <QMap>
#include <QTimer>

#define SETTING DockSettings::instance()
#define XCB XCBUtils::instance()

Dock::Dock(QObject *parent)
 : SynModule(parent)
 , entriesSum(0)
 , windowIdentify(new WindowIdentify(this))
 , entries(new Entries(this))
 , ddeLauncherVisible(false)
 , hideState(HideState::Unknown)
 , activeWindow(nullptr)
 , activeWindowOld(nullptr)
 , dbusHandler(new DBusHandler(this))
 , windowOperateMutex(QMutex(QMutex::NonRecursive))
{
    registeModule("dock");

    QString sessionType {getenv("XDG_SESSION_TYPE")};
    qInfo() << "sessionType=" << sessionType;
    if (sessionType.contains("wayland")) {
        // wayland env
        isWayland = true;
        waylandManager = new WaylandManager(this);
        dbusHandler->listenWaylandWMSignals();
    } else if (sessionType.contains("x11")) {
        // x11 env
        isWayland = false;
        x11Manager = new X11Manager(this);
    }

    initSettings();
    initEntries();

    // 初始化智能隐藏定时器
    smartHideTimer = new QTimer(this);
    smartHideTimer->setSingleShot(true);
    connect(smartHideTimer, &QTimer::timeout, this, &Dock::smartHideModeTimerExpired);

    if (!isWayland) {
        std::thread thread([&] {
            // Xlib方式
            x11Manager->listenXEventUseXlib();
            // XCB方式
            //listenXEventUseXCB();
        });
        thread.detach();
        x11Manager->listenRootWindowXEvent();
        connect(x11Manager, &X11Manager::requestUpdateHideState, this, &Dock::updateHideState);
        connect(x11Manager, &X11Manager::requestHandleActiveWindowChange, this, &Dock::handleActiveWindowChanged);
        connect(x11Manager, &X11Manager::requestAttachOrDetachWindow, this, &Dock::attachOrDetachWindow);
    }

    Q_EMIT serviceRestarted();
}

Dock::~Dock()
{

}

/**
 * @brief Dock::dockEntry 驻留应用
 * @param entry 应用实例
 * @return
 */
bool Dock::dockEntry(Entry *entry)
{
    if (entry->getIsDocked())
        return false;

    AppInfo *app = entry->getApp();
    auto needScratchDesktop = [&]{
        if (!app) {
            qInfo() << "needScratchDesktop: yes, appInfo is nil";
            return true;
        }

        if (app->isInstalled()) {
            qInfo() << "needScratchDesktop: no, desktop is installed";
            return false;
        }

        if (app->getFileName().contains(scratchDir)) {
            qInfo() << "needScratchDesktop: no, desktop in scratchDir";
            return false;
        }

        return true;
    };


    if (needScratchDesktop()) {
        // 创建scratch Desktop file
        QDir dir;
        if (!dir.mkpath(scratchDir)) {
            qWarning() << "create scratch Desktopfile failed";
            return false;
        }

        QFile file;
        QString newDesktopFile;
        if (app) {
            QString newFile = scratchDir + app->getInnerId() + ".desktop";
            if (file.copy(app->getFileName(), newFile))
                newDesktopFile = newFile;
        } else {
            WindowInfoBase *current = entry->getCurrentWindowInfo();
            if (current) {
                QString appId = current->getInnerId();
                QString title = current->getDisplayName();
                QString icon = current->getIcon();
                if (icon.isEmpty())
                    icon = "application-default-icon";

                QString scriptContent = entry->getExec(false);
                QString scriptFile = scratchDir + appId + ".sh";
                file.setFileName(scriptFile);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.write(scriptContent.toStdString().c_str(), scriptContent.size());
                    file.close();
                }

                // createScratchDesktopFile
                QString cmd = scriptFile + " %U";
                QString fileNmae = scratchDir + appId + ".desktop";
                QString desktopContent = QString(dockedItemTemplate).arg(title).arg(cmd).arg(icon);
                file.setFileName(fileNmae);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.write(desktopContent.toStdString().c_str(), desktopContent.size());
                    file.close();
                    newDesktopFile = fileNmae;
                }
            }
        }

        if (newDesktopFile.isEmpty())
            return false;

        app = new AppInfo(newDesktopFile);
        entry->setApp(app);
        entry->updateIcon();
        entry->setInnerId(app->getInnerId());
    }

    entry->setPropIsDocked(true);
    entry->updateMenu();
    return true;
}

/**
 * @brief Dock::undockEntry 取消驻留
 * @param entry 应用实例
 */
void Dock::undockEntry(Entry *entry)
{
    if (!entry->getIsDocked()) {
        qInfo() << "undockEntry: " << entry->getId() << " is not docked";
        return;
    }

    if (!entry->getApp()) {
        qInfo() << "undockEntry: entry appInfo is nullptr";
        return;
    }

    // 移除scratchDir目录下相关文件
    QString desktopFile = entry->getFileName();
    QString filebase(desktopFile.data(), desktopFile.size() - 9);
    if (desktopFile.contains(scratchDir)) {
        for (auto &ext : QStringList() << ".desktop" << ".sh" << ".png") {
            QString fileName = filebase + ext;
            QFile file(fileName);
            if (file.exists())
                file.remove();
        }

    }

    if (entry->hasWindow()) {
        if (desktopFile.contains(scratchDir) && !!entry->getCurrentWindowInfo()) {
            QFileInfo info(desktopFile);
            QString baseName = info.baseName();
            if (baseName.startsWith(windowHashPrefix)) {
                // desktop base starts with w:
                // 由于有 Pid 识别方法在，在这里不能用 m.identifyWindow 再次识别
                entry->setInnerId(entry->getCurrentWindowInfo()->getInnerId());
                entry->setApp(nullptr);  // 此处设置Entry的app为空， 在Entry中调用app相关信息前判断指针是否为空
            } else {
                // desktop base starts with d:
                QString innerId;
                AppInfo *app = windowIdentify->identifyWindow(entry->getCurrentWindowInfo(), innerId);
                // TODO update entry's innerId
                entry->setApp(app);
                entry->setInnerId(innerId);
            }
        }
        entry->updateIcon();
        entry->setPropIsDocked(false);
        entry->updateName();
        entry->updateMenu();
    } else {
        // 直接移除
        removeAppEntry(entry);
    }

    saveDockedApps();
}

/**
 * @brief Dock::allocEntryId 分配应用实例id
 * @return
 */
QString Dock::allocEntryId()
{
    return QString("e%1T%2").arg(++entriesSum).arg(QString::number(QDateTime::currentSecsSinceEpoch(), 16));
}

/**
 * @brief Dock::shouldShowOnDock 判断是否应该显示到任务栏
 * @param info
 * @return
 */
bool Dock::shouldShowOnDock(WindowInfoBase *info)
{
    if (info->getWindowType() == "X11") {
        XWindow winId = info->getXid();
        bool isReg = !!x11Manager->findWindowByXid(winId);
        bool isContainedInClientList = clientList.indexOf(winId) != -1;
        bool shouldSkip = info->shouldSkip();
        bool isGood = XCB->isGoodWindow(winId);
        qInfo() << "shouldShowOnDock X11: isReg:" << isReg << " isContainedInClientList:" << isContainedInClientList << " shouldSkip:" << shouldSkip << " isGood:" << isGood;

       return isReg && isContainedInClientList && isGood && !shouldSkip;
    } else if (info->getWindowType() == "Wayland") {
        return !info->shouldSkip();
    }

    return false;
}

/**
 * @brief Dock::setDdeLauncherVisible 记录当前启动器是否可见
 * @param visible
 */
void Dock::setDdeLauncherVisible(bool visible)
{
    ddeLauncherVisible = visible;
}

/**
 * @brief Dock::getWMName 获取窗管名称
 * @return
 */
QString Dock::getWMName()
{
    return wmName;
}

/**
 * @brief Dock::setWMName 设置窗管名称
 * @param name 窗管名称
 */
void Dock::setWMName(QString name)
{
    wmName = name;
}

/**
 * @brief Dock::setSynConfig 同步配置 TODO
 * @param ba 配置数据
 */
void Dock::setSynConfig(QByteArray ba)
{

}

/**
 * @brief Dock::getSyncConfig 获取配置 TODO
 * @return 配置数据
 */
QByteArray Dock::getSyncConfig()
{
    return QByteArray();
}

/**
 * @brief Dock::createPlasmaWindow 创建wayland下窗口
 * @param objPath
 * @return
 */
PlasmaWindow *Dock::createPlasmaWindow(QString objPath)
{
    return dbusHandler->createPlasmaWindow(objPath);
}

/**
 * @brief Dock::listenKWindowSignals
 * @param windowInfo
 */
void Dock::listenKWindowSignals(WindowInfoK *windowInfo)
{
    dbusHandler->listenKWindowSignals(windowInfo);
}

/**
 * @brief Dock::removePlasmaWindowHandler 关闭窗口后需求对应的connect
 * @param window
 */
void Dock::removePlasmaWindowHandler(PlasmaWindow *window)
{
    dbusHandler->removePlasmaWindowHandler(window);
}

/**
 * @brief Dock::presentWindows 显示窗口
 * @param windows 窗口id
 */
void Dock::presentWindows(QList<uint> windows)
{
    dbusHandler->presentWindows(windows);
}

/**
 * @brief Dock::getDockHideMode 获取任务栏隐藏模式  一直显示/一直隐藏/智能隐藏
 * @return
 */
HideMode Dock::getDockHideMode()
{
    return SETTING->getHideMode();
}

/**
 * @brief Dock::isActiveWindow 判断是否为活动窗口
 * @param win
 * @return
 */
bool Dock::isActiveWindow(const WindowInfoBase *win)
{
    if (!win)
        return false;

    return win == getActiveWindow();
}

/**
 * @brief Dock::getActiveWindow 获取当前活跃窗口
 * @return
 */
WindowInfoBase *Dock::getActiveWindow()
{
    WindowInfoBase *ret = nullptr;
    if (!activeWindow)
        ret = activeWindowOld;
    else
        ret = activeWindow;

    return ret;
}

void Dock::doActiveWindow(XWindow xid)
{
    // 修改当前工作区为指定窗口的工作区
    XWindow winWorkspace = XCB->getWMDesktop(xid);
    XWindow currentWorkspace = XCB->getCurrentWMDesktop();
    if (winWorkspace != currentWorkspace) {
        qInfo() << "doActiveWindow: change currentWorkspace " << currentWorkspace << " to winWorkspace " << winWorkspace;

        // 获取窗口时间
        uint32_t timestamp = XCB->getWMUserTime(xid);
        // 修改当前桌面工作区
        XCB->changeCurrentDesktop(winWorkspace, timestamp);
    }

    XCB->changeActiveWindow(xid);
    QTimer::singleShot(50, [&] {
        XCB->restackWindow(xid);
    });
}

/**
 * @brief Dock::getClientList 获取窗口client列表
 * @return
 */
QList<XWindow> Dock::getClientList()
{
    return QList<XWindow>(clientList);
}

/**
 * @brief Dock::setClientList 设置窗口client列表
 */
void Dock::setClientList(QList<XWindow> value)
{
    clientList = value;
}

/**
 * @brief Dock::closeWindow 关闭窗口
 * @param windowId 窗口id
 */
void Dock::closeWindow(uint32_t windowId)
{
    qInfo() << "Close Window " << windowId;
    if (isWayland) {
        WindowInfoK *info = waylandManager->findWindowByXid(windowId);
        if (info)
            info->close(0);
    } else {
        XCB->requestCloseWindow(windowId, 0);
    }
}

/**
 * @brief Dock::getEntryIDs 获取所有应用Id
 * @return
 */
QStringList Dock::getEntryIDs()
{
    return entries->getEntryIDs();
}

/**
 * @brief Dock::setFrontendWindowRect 设置任务栏Rect
 * @param x
 * @param y
 * @param width
 * @param height
 */
void Dock::setFrontendWindowRect(int32_t x, int32_t y, uint width, uint height)
{
    if (frontendWindowRect == QRect(x, y, width, height)) {
        qInfo() << "SetFrontendWindowRect: no changed";
        return;
    }

    frontendWindowRect.setX(x);
    frontendWindowRect.setY(y);
    frontendWindowRect.setWidth(width);
    frontendWindowRect.setHeight(height);
    updateHideState(false);

    Q_EMIT frontendWindowRectChanged();
}

/**
 * @brief Dock::isDocked 应用是否驻留
 * @param desktopFile
 * @return
 */
bool Dock::isDocked(const QString desktopFile)
{
    auto entry = getDockedEntryByDesktopFile(desktopFile);
    return !!entry;
}

/**
 * @brief Dock::requestDock 驻留应用
 * @param desktopFile desktopFile全路径
 * @param index
 * @return
 */
bool Dock::requestDock(QString desktopFile, int index)
{
    qInfo() << "RequestDock: " << desktopFile;
    AppInfo *app = new AppInfo(desktopFile);
    if (!app || !app->isValidApp()) {
        qInfo() << "RequestDock: invalid desktopFile";
        return false;
    }

    bool newCreated = false;
    Entry *entry = entries->getByInnerId(app->getInnerId());
    if (!entry) {
        newCreated = true;
        entry = new Entry(this, app, app->getInnerId());
    }

    if (!dockEntry(entry))
        return false;

    if (newCreated) {
        entry->startExport();
        entries->append(entry);
    }

    saveDockedApps();
    return true;
}

/**
 * @brief Dock::requestUndock 取消驻留应用
 * @param desktopFile desktopFile文件全路径
 * @return
 */
bool Dock::requestUndock(QString desktopFile)
{
   auto entry = getDockedEntryByDesktopFile(desktopFile);
   if (!entry)
       return false;

   undockEntry(entry);
   return true;
}

/**
 * @brief Dock::moveEntry 移动驻留程序顺序
 * @param oldIndex
 * @param newIndex
 */
void Dock::moveEntry(int oldIndex, int newIndex)
{
    entries->move(oldIndex, newIndex);
    saveDockedApps();
}

/**
 * @brief Dock::isOnDock 是否在任务栏
 * @param desktopFile desktopFile文件全路径
 * @return
 */
bool Dock::isOnDock(QString desktopFile)
{
    return !!entries->getByDesktopFilePath(desktopFile);
}

/**
 * @brief Dock::queryWindowIdentifyMethod 查询窗口识别方式
 * @param windowId 窗口id
 * @return
 */
QString Dock::queryWindowIdentifyMethod(XWindow windowId)
{
    return entries->queryWindowIdentifyMethod(windowId);
}

/**
 * @brief Dock::getDockedAppsDesktopFiles 获取驻留应用desktop文件
 * @return
 */
QStringList Dock::getDockedAppsDesktopFiles()
{
    QStringList ret;
    for (auto entry: entries->filterDockedEntries()) {
        ret << entry->getFileName();
    }

    return ret;
}

/**
 * @brief Dock::getPluginSettings 获取任务栏插件配置
 * @return
 */
QString Dock::getPluginSettings()
{
    return SETTING->getPluginSettings();
}

/**
 * @brief Dock::setPluginSettings 设置任务栏插件配置
 * @param jsonStr
 */
void Dock::setPluginSettings(QString jsonStr)
{
    SETTING->setPluginSettings(jsonStr);
}

/**
 * @brief Dock::mergePluginSettings 合并任务栏插件配置
 * @param jsonStr
 */
void Dock::mergePluginSettings(QString jsonStr)
{
    SETTING->mergePluginSettings(jsonStr);
}

/**
 * @brief Dock::removePluginSettings 移除任务栏插件配置
 * @param pluginName
 * @param settingkeys
 */
void Dock::removePluginSettings(QString pluginName, QStringList settingkeys)
{
    SETTING->removePluginSettings(pluginName, settingkeys);
}

/**
 * @brief Dock::smartHideModeTimerExpired 设置智能隐藏
 */
void Dock::smartHideModeTimerExpired()
{
    HideState state = shouldHideOnSmartHideMode() ? HideState::Hide : HideState::Show;
    qInfo() << "smartHideModeTimerExpired, should hide ? " << int(state);
    setPropHideState(state);
}

/**
 * @brief Dock::initSettings 初始化配置
 */
void Dock::initSettings()
{
    qInfo() << "init dock settings";
    forceQuitAppStatus = SETTING->getForceQuitAppMode();
    connect(SETTING, &DockSettings::hideModeChanged, this, [&](HideMode mode) {
        this->updateHideState(false);
    });
    connect(SETTING, &DockSettings::displayModeChanged, this, [](DisplayMode mode) {
       qInfo() << "display mode change to " << static_cast<int>(mode);
    });
    connect(SETTING, &DockSettings::positionModeChanged, this, [](PositionMode mode) {
       qInfo() << "position mode change to " << static_cast<int>(mode);
    });
    connect(SETTING, &DockSettings::forceQuitAppChanged, this, [&](ForceQuitAppMode mode) {
       qInfo() << "forceQuitApp change to " << int(mode);
       forceQuitAppStatus = mode;
       entries->updateEntriesMenu();
    });
}

/**
 * @brief Dock::updateMenu 更新所有应用菜单 TODO
 */
void Dock::updateMenu()
{

}

/**
 * @brief Dock::initEntries 初始化应用
 */
void Dock::initEntries()
{
    initDockedApps();
    if (!isWayland)
        initClientList();
}

/**
 * @brief Dock::initDockedApps 初始化驻留应用
 */
void Dock::initDockedApps()
{
    // 初始化驻留应用信息
    for (const auto &app : SETTING->getDockedApps()) {
        if (app.isEmpty() || app[0] != '/' || app.size() <= 3)
            continue;

        QString prefix(app.data(), 3);
        QString appId(app.data() + 3);
        QString path;
        if (pathCodeDirMap.find(prefix) != pathCodeDirMap.end())
            path = pathCodeDirMap[prefix] + appId + ".desktop";

        DesktopInfo info(path.toStdString());
        if (!info.isValidDesktop())
            continue;

        AppInfo *appInfo = new AppInfo(info);
        Entry *entryObj = new Entry(this, appInfo, appInfo->getInnerId());
        entryObj->setIsDocked(true);
        entryObj->updateMenu();
        entryObj->startExport();
        entries->append(entryObj);
    }

    saveDockedApps();
}

/**
 * @brief Dock::initClientList 初始化窗口列表，关联到对应应用
 */
void Dock::initClientList()
{
    QList<XWindow> clients;
    for (auto c : XCB->instance()->getClientList())
        clients.push_back(c);

    // 依次注册窗口
    qSort(clients.begin(), clients.end());
    clientList = clients;
    for (auto winId : clientList) {
        WindowInfoX *winInfo = x11Manager->registerWindow(winId);
        attachOrDetachWindow(static_cast<WindowInfoBase *>(winInfo));
    }
}

/**
 * @brief Dock::findWindowByXidX 通过id获取窗口信息
 * @param xid
 * @return
 */
WindowInfoX *Dock::findWindowByXidX(XWindow xid)
{
    return x11Manager->findWindowByXid(xid);
}

/**
 * @brief Dock::findWindowByXidK 通过xid获取窗口  TODO wayland和x11下窗口尽量完全剥离， 不应该存在通过xid查询wayland窗口的情况
 * @param xid
 * @return
 */
WindowInfoK *Dock::findWindowByXidK(XWindow xid)
{
    return waylandManager->findWindowByXid(xid);
}

/**
 * @brief Dock::isWindowDockOverlapX 判断X环境下窗口和任务栏是否重叠
 * @param xid
 * @return
 * 计算重叠条件：
 * 1 窗口类型非桌面desktop
 * 2 窗口透明度非0
 * 3 窗口显示在当前工作区域
 * 4 窗口和任务栏rect存在重叠区域
 */
bool Dock::isWindowDockOverlapX(XWindow xid)
{
    // 检查窗口类型
    auto desktopType = XCB->getAtom("_NET_WM_WINDOW_TYPE_DESKTOP");
    for (auto ty : XCB->getWMWindoType(xid)) {
        if (ty == desktopType) {
            // 不处理桌面窗口属性
            return false;
        }
    }

    // TODO 检查窗口透明度
    // 检查窗口是否显示
    auto wmHiddenType = XCB->getAtom("_NET_WM_STATE_HIDDEN");
    for (auto ty : XCB->getWMState(xid)) {
        if (ty == wmHiddenType) {
            // 不处理隐藏的窗口属性
            return false;
        }
    }

    // 检查窗口是否在当前工作区
    uint32_t wmDesktop = XCB->getWMDesktop(xid);
    uint32_t currentDesktop = XCB->getCurrentWMDesktop();
    if (wmDesktop != currentDesktop) {
        qInfo() << "isWindowDockOverlapX: wmDesktop:" << wmDesktop << " is not equal to currentDesktop:" << currentDesktop;
        return false;
    }

    // 检查窗口和任务栏窗口是否存在重叠
    auto winRect = XCB->getWindowGeometry(xid);
    return hasInterSectionX(winRect, frontendWindowRect);
}

/**
 * @brief Dock::hasInterSectionX 检查窗口重叠区域
 * @param windowRect 活动窗口
 * @param dockRect  任务栏窗口
 * @return
 */
bool Dock::hasInterSectionX(const Geometry &windowRect, QRect dockRect)
{
    int ltX = MAX(windowRect.x, dockRect.x());
    int ltY = MAX(windowRect.y, dockRect.y());
    int rbX = MIN(windowRect.x + windowRect.width, dockRect.x() + dockRect.width());
    int rbY = MIN(windowRect.y + windowRect.height, dockRect.y() + dockRect.height());

    return (ltX < rbX) && (ltY < rbY);
}

/**
 * @brief Dock::isWindowDockOverlapK 判断Wayland环境下窗口和任务栏是否重叠
 * @param info
 * @return
 */
bool Dock::isWindowDockOverlapK(WindowInfoBase *info)
{
    WindowInfoK *infoK = static_cast<WindowInfoK *>(info);
    if (!infoK) {
        qInfo() << "isWindowDockOverlapK: infoK is nullptr";
        return false;
    }

    DockRect rect = infoK->getGeometry();
    bool isActiveWin = infoK->getPlasmaWindow()->IsActive();
    QString appId = infoK->getAppId();
    if (!isActiveWin) {
        qInfo() << "isWindowDockOverlapK: check window " << appId << " is not active";
        return false;
    }

    QStringList appList = {"dde-desktop", "dde-lock", "dde-shutdown"};
    if (appList.contains(appId)) {
        qInfo() << "isWindowDockOverlapK: appId in white list";
        return false;
    }

    return hasInterSectionK(rect, frontendWindowRect);
}

/**
 * @brief Dock::hasInterSectionK Wayland环境下判断活动窗口和任务栏区域是否重叠
 * @param windowRect 活动窗口
 * @param dockRect 任务栏窗口
 * @return
 */
bool Dock::hasInterSectionK(const DockRect &windowRect, QRect dockRect)
{
    int position = getPosition();
    int ltX = MAX(windowRect.X, dockRect.x());
    int ltY = MAX(windowRect.Y, dockRect.y());
    int rbX = MIN(windowRect.X + windowRect.Width, dockRect.x() + dockRect.width());
    int rbY = MIN(windowRect.Y + windowRect.Height, dockRect.y() + dockRect.height());

    if (position == int(PositionMode::Left) || position == int(PositionMode::Right)) {
        return ltX <= rbX && ltY < rbY;
    } else if (position == int(PositionMode::Top) || position == int(PositionMode::Bottom)) {
        return ltX < rbX && ltY <= rbY;
    } else {
        return ltX < rbX && ltY < rbY;
    }
}

/**
 * @brief Dock::getDockedEntryByDesktopFile 获取应用实例
 * @param desktopFile desktopFile文件全路径
 * @return
 */
Entry *Dock::getDockedEntryByDesktopFile(const QString &desktopFile)
{
    return entries->getDockedEntryByDesktopFile(desktopFile);
}

/**
 * @brief Dock::shouldHideOnSmartHideMode 判断智能隐藏模式下当前任务栏是否应该隐藏
 * @return
 */
bool Dock::shouldHideOnSmartHideMode()
{
    if (!activeWindow || ddeLauncherVisible)
        return false;

    if (!isWayland) {
        XWindow activeWinId = activeWindow->getXid();

        // dde launcher is invisible, but it is still active window
        WMClass winClass = XCB->getWMClass(activeWinId);
        if (winClass.instanceName.size() > 0 && winClass.instanceName.c_str() == ddeLauncherWMClass) {
            qInfo() << "shouldHideOnSmartHideMode: active window is dde launcher";
            return false;
        }

        QVector<XWindow> list = getActiveWinGroup(activeWinId);
        for (XWindow xid : list) {
            if (isWindowDockOverlapX(xid)) {
                qInfo() << "shouldHideOnSmartHideMode: window has overlap";
                return true;
            }
        }
        return false;
    } else {
        return isWindowDockOverlapK(activeWindow);
    }
}

/**
 * @brief Dock::getActiveWinGroup
 * @param xid
 * @return
 */
QVector<XWindow> Dock::getActiveWinGroup(XWindow xid)
{
    QVector<XWindow> ret;
    ret.push_back(xid);

    std::list<XWindow> winList = XCB->getClientListStacking();
    if (winList.empty()
            || !std::any_of(winList.begin(), winList.end(), [&](XWindow id) { return id == xid;}) // not found active window in clientListStacking"
        ||  *winList.begin() == 0) // root window
        return ret;

    uint32_t apid = XCB->getWMPid(xid);
    XWindow aleaderWin = XCB->getWMClientLeader(xid);
    for (auto winId : winList) {
        if (winId == xid)
            break;

        uint32_t pid = XCB->getWMPid(winId);
        // same pid
        if (apid != 0 && pid == apid) {
            // ok
            ret.push_back(winId);
            continue;
        }

        WMClass wmClass = XCB->getWMClass(winId);
        // same wmclass
        if (wmClass.className.size() > 0 && wmClass.className.c_str() == frontendWindowWmClass) {
            // skip over fronted window
            continue;
        }

        uint32_t leaderWin = XCB->getWMClientLeader(winId);
        // same leaderWin
        if (aleaderWin != 0 && aleaderWin == leaderWin) {
            // ok
            ret.push_back(winId);
            continue;
        }

        // above window
        XWindow aboveWinId = 0;
        for (auto iter = winList.begin(); iter != winList.end(); iter++) {
            if (*iter == winId) {
                aboveWinId = *++iter;
                break;
            }
        }

        if (aboveWinId == 0)
            continue;

        XWindow aboveWinTransientFor = XCB->getWMTransientFor(aboveWinId);
        if (aboveWinTransientFor != 0 && aboveWinTransientFor == winId) {
            // ok
            ret.push_back(winId);
            continue;
        }
    }

    return ret;
}

/**
 * @brief Dock::updateHideState 更新任务栏隐藏状态
 * @param delay
 */
void Dock::updateHideState(bool delay)
{
    if (ddeLauncherVisible) {
        qInfo() << "updateHideState: dde launcher is visible, show dock";
        setPropHideState(HideState::Show);
    }

    HideMode mode = SETTING->getHideMode();
    switch (mode) {
    case HideMode::KeepShowing:
        setPropHideState(HideState::Show);
        break;
    case HideMode::KeepHidden:
        setPropHideState(HideState::Hide);
        break;
    case HideMode::SmartHide:
        qInfo() << "reset smart hide mode timer " << delay;
        smartHideTimer->start(delay ? smartHideTimerDelay : 0);
        break;
    }
}

/**
 * @brief Dock::setPropHideMode 设置隐藏属性
 * @param state
 */
void Dock::setPropHideState(HideState state)
{
    if (state == HideState::Unknown) {
        qInfo() << "setPropHideState: unknown mode";
        return;
    }

    if (state != hideState) {
        hideState = state;
        Q_EMIT hideStateChanged(static_cast<int>(hideState));
    }
}

/**
 * @brief Dock::attachOrDetachWindow 关联或分离窗口
 * @param info
 */
void Dock::attachOrDetachWindow(WindowInfoBase *info)
{
    if (!info)
        return;

    XWindow winId = info->getXid();
    bool shouldDock = shouldShowOnDock(info);
    qInfo() << "attachOrDetachWindow: shouldDock " << shouldDock;

    // 顺序解析窗口合并或分离操作
    QMutexLocker locker(&windowOperateMutex);
    Entry *entry = info->getEntry();
    if (entry) {
        // detach
        if (!shouldDock)
            detachWindow(info);
        else
            qInfo() << "detach operate: window " << winId << "don't need to detach";
    } else {
        // attach
        if (info->getEntryInnerId().isEmpty()) {
            // 窗口entryInnerId为空表示未识别，需要识别窗口并创建entryInnerId
            qInfo() << "attach operate: window " << winId << " entryInnerId is empty, now call IdentifyWindow";
            QString innerId;
            AppInfo *appInfo = windowIdentify->identifyWindow(info, innerId);
            // 窗口entryInnerId即AppInfo的innerId， 用来将窗口和应用绑定关系
            info->setEntryInnerId(innerId);
            info->setAppInfo(appInfo);
            markAppLaunched(appInfo);
        } else {
            qInfo() << "attach operate: window " << winId << "has been identified";
        }

        // winInfo初始化后影响判断是否在任务栏显示图标，需判断
        if (shouldShowOnDock(info))
            attachWindow(info);
    }
}

/**
 * @brief Dock::attachWindow 关联窗口
 * @param info 窗口信息
 */
void Dock::attachWindow(WindowInfoBase *info)
{
    // TODO: entries中存在innerid为空的entry， 导致后续新应用通过innerid获取应用一直能获取到
    Entry *entry = entries->getByInnerId(info->getEntryInnerId());
    if (entry) {
        // entry existed
        entry->attachWindow(info);
    } else {
        entry = new Entry(this, info->getAppInfo(), info->getEntryInnerId());
        if (entry->attachWindow(info)) {
            entry->startExport();
            entries->append(entry);
        }
    }
}

/**
 * @brief Dock::detachWindow 分离窗口
 * @param info 窗口信息
 */
void Dock::detachWindow(WindowInfoBase *info)
{
    auto entry = entries->getByWindowId(info->getXid());
    if (!entry)
        return;

    bool needRemove = entry->detachWindow(info);
    if (needRemove)
        removeAppEntry(entry);
}

/**
 * @brief Dock::launchApp 启动应用
 * @param timestamp 时间
 * @param files 应用打开文件
 */
void Dock::launchApp(const QString desktopFile, uint32_t timestamp, QStringList files)
{
    dbusHandler->launchApp(desktopFile, timestamp, files);
}

/**
 * @brief Dock::launchAppAction 启动应用响应
 * @param timestamp
 * @param file
 * @param section
 */
void Dock::launchAppAction(const QString desktopFile, QString action, uint32_t timestamp)
{
    dbusHandler->launchAppAction(desktopFile, action, timestamp);
}

/**
 * @brief Dock::is3DWM 当前窗口模式 2D/3D
 * @return
 */
bool Dock::is3DWM()
{
    bool ret = false;
    if  (wmName.isEmpty())
        wmName = dbusHandler->getCurrentWM();

    if (wmName == "deepin wm")
        ret = true;

    return ret;
}

/**
 * @brief Dock::isWaylandEnv 当前环境
 * @return
 */
bool Dock::isWaylandEnv()
{
    return isWayland;
}

/**
 * @brief Dock::handleActiveWindowChangedK 处理活动窗口改变事件 wayland环境
 * @param activeWin
 * @return
 */
WindowInfoK *Dock::handleActiveWindowChangedK(uint activeWin)
{
    return waylandManager->handleActiveWindowChangedK(activeWin);
}

/**
 * @brief Dock::handleActiveWindowChanged 处理活动窗口改变事件 X11环境
 * @param info
 */
void Dock::handleActiveWindowChanged(WindowInfoBase *info)
{
    qInfo() << "handleActiveWindowChanged";
    if (!info) {
        activeWindowOld = info;
        activeWindow = nullptr;
        return;
    }

    activeWindow = info;
    XWindow winId = activeWindow->getXid();
    entries->handleActiveWindowChanged(winId);
    updateHideState(true);
}

/**
 * @brief Dock::saveDockedApps 保存驻留应用信息
 */
void Dock::saveDockedApps()
{
    QList<QString> dockedApps;
    for (auto entry : entries->filterDockedEntries()) {
        QString path = entry->getApp()->getFileName();
        for (auto iter=pathDirCodeMap.begin(); iter != pathDirCodeMap.end(); iter++) {
            if (path.startsWith(iter.key())) {
                path = QString(path.data() + iter.key().size()); // 去头dir
                path.truncate(path.size() - 8); // 去尾.desktop
                dockedApps.push_back(iter.value() + path);
                break;
            }
        }
    }
    SETTING->setDockedApps(dockedApps);
}

/** 移除应用实例
 * @brief Dock::removeAppEntry
 * @param entry
 */
void Dock::removeAppEntry(Entry *entry)
{
    if (entry) {
        entries->remove(entry);
    }
}

/**
 * @brief Dock::handleWindowGeometryChanged 智能隐藏模式下窗口矩形变化，同步更新任务栏隐藏状态
 */
void Dock::handleWindowGeometryChanged()
{
    if (SETTING->getHideMode() == HideMode::SmartHide)
        return;

    updateHideState(false);
}

/**
 * @brief Dock::getEntryByWindowId 根据窗口id获取应用实例
 * @param windowId
 * @return
 */
Entry *Dock::getEntryByWindowId(XWindow windowId)
{
    return entries->getByWindowId(windowId);
}

/**
 * @brief Dock::getDesktopFromWindowByBamf 通过bamf软件服务获取指定窗口的desktop文件
 * @param windowId
 * @return
 */
QString Dock::getDesktopFromWindowByBamf(XWindow windowId)
{
    return dbusHandler->getDesktopFromWindowByBamf(windowId);
}

/**
 * @brief Dock::registerWindowWayland 注册wayland窗口
 * @param objPath
 */
void Dock::registerWindowWayland(const QString &objPath)
{
    return waylandManager->registerWindow(objPath);
}

/**
 * @brief Dock::unRegisterWindowWayland 取消注册wayland窗口
 * @param objPath
 */
void Dock::unRegisterWindowWayland(const QString &objPath)
{
    return waylandManager->unRegisterWindow(objPath);
}

/**
 * @brief Dock::isShowingDesktop
 * @return
 */
bool Dock::isShowingDesktop()
{
    return dbusHandler->wlShowingDesktop();
}

/**
 * @brief Dock::identifyWindow 识别窗口
 * @param winInfo
 * @param innerId
 * @return
 */
AppInfo *Dock::identifyWindow(WindowInfoBase *winInfo, QString &innerId)
{
    return windowIdentify->identifyWindow(winInfo, innerId);
}

/**
 * @brief Dock::markAppLaunched 标识应用已启动
 * @param appInfo
 */
void Dock::markAppLaunched(AppInfo *appInfo)
{
    if (!appInfo || !appInfo->isValidApp())
        return;

    QString desktopFile = appInfo->getFileName();
    qInfo() << "markAppLaunched: desktopFile is " << desktopFile;
    dbusHandler->markAppLaunched(desktopFile);
}

/**
 * @brief Dock::deleteWindow 删除窗口  TODO 检查必要性
 * @param xid
 */
void Dock::deleteWindow(XWindow xid)
{
    entries->deleteWindow(xid);
}

/**
 * @brief Dock::getForceQuitAppStatus 获取强制关闭应用状态
 * @return
 */
ForceQuitAppMode Dock::getForceQuitAppStatus()
{
    return forceQuitAppStatus;
}

/**
 * @brief Dock::getWinIconPreferredApps 获取推荐的应用窗口图标
 * @return
 */
QVector<QString> Dock::getWinIconPreferredApps()
{
    return SETTING->getWinIconPreferredApps();
}

/**
 * @brief Dock::handleLauncherItemDeleted 处理launcher item被删除信号
 * @param itemPath
 */
void Dock::handleLauncherItemDeleted(QString itemPath)
{
    for (auto entry : entries->filterDockedEntries()) {
        if (entry->getFileName() == itemPath) {
            undockEntry(entry);
            break;
        }
    }
}

/**
 * @brief Dock::handleLauncherItemUpdated 在收到 launcher item 更新的信号后，需要更新相关信息，包括 appInfo、innerId、名称、图标、菜单。
 * @param itemPath
 */
void Dock::handleLauncherItemUpdated(QString itemPath)
{
    Entry * entry = entries->getByDesktopFilePath(itemPath);
    if (!entry)
        return;

    AppInfo *app = new AppInfo(itemPath);
    entry->setApp(app);
    entry->setInnerId(app->getInnerId());
    entry->updateName();
    entry->updateMenu();
    entry->forceUpdateIcon(); // 可能存在Icon图片改变,但Icon名称未改变的情况,因此强制发Icon的属性改变信号
}

/**
 * @brief Dock::getOpacity 获取透明度
 * @return
 */
double Dock::getOpacity()
{
    return SETTING->getOpacity();
}

/**
 * @brief Dock::getFrontendWindowRect 获取任务栏rect
 * @return
 */
QRect Dock::getFrontendWindowRect()
{
    return frontendWindowRect;
}

/**
 * @brief Dock::getDisplayMode 获取显示模式
 * @return
 */
int Dock::getDisplayMode()
{
    return int(SETTING->getDisplayMode());
}

/**
 * @brief Dock::setDisplayMode 设置显示模式
 * @param mode
 */
void Dock::setDisplayMode(int mode)
{
    SETTING->setDisplayMode(DisplayMode(mode));
}

/**
 * @brief Dock::getDockedApps 获取驻留应用
 * @return
 */
QStringList Dock::getDockedApps()
{
    return SETTING->getDockedApps();
}

/**
 * @brief Dock::getEntryPaths 获取应用实例路径
 * @return
 */
QStringList Dock::getEntryPaths()
{
    QStringList ret;
    for (auto id : entries->getEntryIDs()) {
        ret.push_back(entryDBusObjPathPrefix + id);
    }

    return ret;
}

/**
 * @brief Dock::getHideMode 获取隐藏模式
 * @return
 */
HideMode Dock::getHideMode()
{
    return SETTING->getHideMode();
}

/**
 * @brief Dock::setHideMode 设置隐藏模式
 * @param mode
 */
void Dock::setHideMode(HideMode mode)
{
    SETTING->setHideMode(mode);
}

/**
 * @brief Dock::getHideState 获取隐藏状态
 * @return
 */
HideState Dock::getHideState()
{
    return hideState;
}

/**
 * @brief Dock::setHideState 设置任务栏隐藏状态
 * @param state
 */
void Dock::setHideState(HideState state)
{
    hideState = state;
}

/**
 * @brief Dock::getHideTimeout 获取执行隐藏动作超时时间
 * @return
 */
uint Dock::getHideTimeout()
{
    return SETTING->getHideTimeout();
}

/**
 * @brief Dock::setHideTimeout 设置执行隐藏动作超时时间
 * @param timeout
 */
void Dock::setHideTimeout(uint timeout)
{
    SETTING->setHideTimeout(timeout);
}

/**
 * @brief Dock::getIconSize 获取应用图标大小
 * @return
 */
uint Dock::getIconSize()
{
    return SETTING->getIconSize();
}

/**
 * @brief Dock::setIconSize 设置应用图标大小
 * @param size
 */
void Dock::setIconSize(uint size)
{
    SETTING->setIconSize(size);
}

/**
 * @brief Dock::getPosition 获取当前任务栏位置
 * @return
 */
int Dock::getPosition()
{
    return int(SETTING->getPositionMode());
}

/**
 * @brief Dock::setPosition 设置任务栏位置
 * @param position
 */
void Dock::setPosition(int position)
{
    SETTING->setPositionMode(PositionMode(position));
}

/**
 * @brief Dock::getShowTimeout 获取显示超时接口
 * @return
 */
uint Dock::getShowTimeout()
{
    return SETTING->getShowTimeout();
}

/**
 * @brief Dock::setShowTimeout 设置显示超时
 * @param timeout
 */
void Dock::setShowTimeout(uint timeout)
{
    return SETTING->setShowTimeout(timeout);
}

/**
 * @brief Dock::getWindowSizeEfficient 获取任务栏高效模式大小
 * @return
 */
uint Dock::getWindowSizeEfficient()
{
    return SETTING->getWindowSizeEfficient();
}

/**
 * @brief Dock::setWindowSizeEfficient 设置任务栏高效模式大小
 * @param size
 */
void Dock::setWindowSizeEfficient(uint size)
{
    SETTING->setWindowSizeEfficient(size);
}

/**
 * @brief Dock::getWindowSizeFashion 获取任务栏时尚模式大小
 * @return
 */
uint Dock::getWindowSizeFashion()
{
    return SETTING->getWindowSizeFashion();
}

/**
 * @brief Dock::setWindowSizeFashion 设置任务栏时尚模式大小
 * @param size
 */
void Dock::setWindowSizeFashion(uint size)
{
    SETTING->setWindowSizeFashion(size);
}

