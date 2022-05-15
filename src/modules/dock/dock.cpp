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
#include "common.h"
#include "windowinfok.h"
#include "xcbutils.h"
#include "dbushandler.h"
#include "appinfo.h"
#include "waylandmanager.h"
#include "x11manager.h"
#include "windowinfobase.h"
#include "dbusplasmawindow.h"

#include <QDir>
#include <QMap>
#include <QTimer>

#define SETTING DockSettings::instance()
#define XCB XCBUtils::instance()

Dock::Dock(QObject *parent)
 : SynModule(parent)
 , windowIdentify(new WindowIdentify(this))
 , entries(new Entries(this))
 , ddeLauncherVisible(true)
 , hideState(HideState::Unknown)
 , activeWindow(nullptr)
 , activeWindowOld(nullptr)
 , dbusHandler(new DBusHandler(this))
 , windowOperateMutex(QMutex(QMutex::NonRecursive))
{
    registeModule("dock");

    QString sessionType {getenv("XDG_SESSION_TYPE")};
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
    smartHideTimer->setInterval(10 * 1000);
    connect(smartHideTimer, SIGNAL(timeout()), this, SLOT(smartHideModeTimerExpired)); // 增加开始判断
    smartHideTimer->stop();

    if (!isWayland) {
        std::thread thread([&] {
            // Xlib方式
            x11Manager->listenXEventUseXlib();
            // XCB方式
            //listenXEventUseXCB();
        });
        thread.detach();
        x11Manager->listenRootWindowXEvent();
        connect(x11Manager, SIGNAL(X11Manager::requestUpdateHideState), this, SLOT(updateHideState));
    }
    Q_EMIT serviceRestarted();
}

Dock::~Dock()
{

}

// 驻留
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

// 取消驻留
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

QString Dock::allocEntryId()
{
    int num = entriesSum++;
    return QString::number(num);
}

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

void Dock::setDdeLauncherVisible(bool visible)
{
    ddeLauncherVisible = visible;
}

QString Dock::getWMName()
{
    return wmName;
}

void Dock::setWMName(QString name)
{
    wmName = name;
}

void Dock::setSynConfig(QByteArray ba)
{

}

QByteArray Dock::getSyncConfig()
{
    return QByteArray();
}

PlasmaWindow *Dock::createPlasmaWindow(QString objPath)
{
    return dbusHandler->createPlasmaWindow(objPath);
}

void Dock::listenKWindowSignals(WindowInfoK *windowInfo)
{
    dbusHandler->listenKWindowSignals(windowInfo);
}

void Dock::removePlasmaWindowHandler(PlasmaWindow *window)
{
    dbusHandler->removePlasmaWindowHandler(window);
}

void Dock::presentWindows(QList<uint> windows)
{
    dbusHandler->presentWindows(windows);
}

HideMode Dock::getDockHideMode()
{
    return SETTING->getHideMode();
}

WindowInfoBase *Dock::getActiveWindow()
{
    WindowInfoBase *ret = nullptr;
    if (!activeWindow)
        ret = activeWindowOld;
    else
        ret = activeWindow;

    return ret;
}

QList<XWindow> Dock::getClientList()
{
    return QList<XWindow>(clientList);
}

// 关闭窗口
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

// 获取所有应用Id for debug
QStringList Dock::getEntryIDs()
{
    return entries->getEntryIDs();
}

// 设置任务栏Rect
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

// 应用是否驻留
bool Dock::isDocked(const QString desktopFile)
{
    auto entry = getDockedEntryByDesktopFile(desktopFile);
    return !!entry;
}

// 驻留应用
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

bool Dock::requestUndock(QString desktopFile)
{
   auto entry = getDockedEntryByDesktopFile(desktopFile);
   if (!entry)
       return false;

   undockEntry(entry);
   return true;
}

// 移动驻留程序顺序
void Dock::moveEntry(int oldIndex, int newIndex)
{
    entries->move(oldIndex, newIndex);
    saveDockedApps();
}

// 是否在任务栏
bool Dock::isOnDock(QString desktopFile)
{
    return !!getDockedEntryByDesktopFile(desktopFile);
}

// 查询窗口识别方式
QString Dock::queryWindowIdentifyMethod(XWindow windowId)
{
    return entries->queryWindowIdentifyMethod(windowId);
}

// 获取驻留应用desktop文件
QStringList Dock::getDockedAppsDesktopFiles()
{
    QStringList ret;
    for (auto entry: entries->filterDockedEntries()) {
        ret << entry->getFileName();
    }

    return ret;
}

// 获取任务栏插件配置
QString Dock::getPluginSettings()
{
    return SETTING->getPluginSettings();
}

// 设置任务栏插件配置
void Dock::setPluginSettings(QString jsonStr)
{
    SETTING->setPluginSettings(jsonStr);
}

// 合并任务栏插件配置
void Dock::mergePluginSettings(QString jsonStr)
{
    SETTING->mergePluginSettings(jsonStr);
}

// 移除任务栏插件配置
void Dock::removePluginSettings(QString pluginName, QStringList settingkeys)
{
    SETTING->removePluginSettings(pluginName, settingkeys);
}

// 智能隐藏
void Dock::smartHideModeTimerExpired()
{
    HideState state = shouldHideOnSmartHideMode() ? HideState::Hide : HideState::Show;
    qInfo() << "smartHideModeTimerExpired, is Hide? " << int(state);
    setPropHideMode(state);
}

void Dock::initSettings()
{
    SETTING->init();
    connect(SETTING, &DockSettings::hideModeChanged, this, [&](HideMode mode) {
        this->updateHideState(false);
    });
    connect(SETTING, &DockSettings::displayModeChanged, this, [](DisplayMode mode) {
       qInfo() << "display mode change to " << static_cast<int>(mode);
    });
    connect(SETTING, &DockSettings::positionModeChanged, this, [](PositonMode mode) {
       qInfo() << "position mode change to " << static_cast<int>(mode);
    });
    connect(SETTING, &DockSettings::forceQuitAppChanged, this, [&](bool mode) {
       qInfo() << "forceQuitApp change to " << mode;
       entries->updateEntriesMenu();
    });
}

// 更新所有应用菜单
void Dock::updateMenu()
{

}

// 初始化应用
void Dock::initEntries()
{
    initDockedApps();
    if (!isWaylandEnv())
        initClientList();
}

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

#include "processinfo.h"
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

WindowInfoX *Dock::findWindowByXidX(XWindow xid)
{
    return x11Manager->findWindowByXid(xid);
}

WindowInfoK *Dock::findWindowByXidK(XWindow xid)
{
    return waylandManager->findWindowByXid(xid);
}

bool Dock::isWindowDockOverlapX(XWindow xid)
{

    return false;
}

// 判断窗口和任务栏是否重叠
bool Dock::isWindowDockOverlapK(WindowInfoBase *info)
{
    if (!info) {
        qInfo() << "isWindowDockOverlapK: info is nullptr";
        return false;
    }

    //TODO check

    return false;
}

Entry *Dock::getDockedEntryByDesktopFile(const QString &desktopFile)
{
    return entries->getDockedEntryByDesktopFile(desktopFile);
}

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
            if (isWindowDockOverlapX(xid))
                return true;
        }
        return false;
    } else {
        return isWindowDockOverlapK(activeWindow);
    }
}

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

// 更新任务栏隐藏状态
void Dock::updateHideState(bool delay)
{
    if (ddeLauncherVisible) {
        qInfo() << "updateHideState: dde launcher is visible, show dock";
        setPropHideMode(HideState::Show);
    }

    HideMode mode = SETTING->getHideMode();
    switch (mode) {
    case HideMode::KeepShowing:
        setPropHideMode(HideState::Show);
        break;
    case HideMode::KeepHidden:
        setPropHideMode(HideState::Hide);
        break;
    case HideMode::SmartHide:
        qInfo() << "reset smart hide mode timer " << delay;
        smartHideTimer->start(delay ? smartHideTimerDelay : 0);
        break;
    case HideMode::Unknown:
        setPropHideMode(HideState::Unknown);
        break;
    }
}

void Dock::setPropHideMode(HideState state)
{
    if (state == HideState::Unknown) {
        qInfo() << "setPropHideState: unknown mode";
        return;
    }

    if (state != hideState) {
        hideState = state;
        Q_EMIT hideStateChanged();
    }
}

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
            // 识别窗口并创建entryInnerId
            qInfo() << "attach operate: window " << winId << " entryInnerId is empty, now call IdentifyWindow";
            QString innerId;
            AppInfo *appInfo = windowIdentify->identifyWindow(info, innerId);
            info->setEntryInnerId(innerId);   // windowBaseInfo entryInnerId is AppInfo innerId, for binding window and appInfo
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

void Dock::attachWindow(WindowInfoBase *info)
{
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

void Dock::detachWindow(WindowInfoBase *info)
{
    auto entry = entries->getByWindowId(info->getXid());
    if (!entry)
        return;

    bool needRemove = entry->detachWindow(info);
    if (needRemove)
        removeAppEntry(entry);
}

void Dock::markAppLaunched(const QString &filePath)
{
    dbusHandler->markAppLaunched(filePath);
}

void Dock::launchApp(uint32_t timestamp, QStringList files)
{
    dbusHandler->launchApp(timestamp, files);
}

void Dock::launchAppAction(uint32_t timestamp, QString file, QString section)
{
    dbusHandler->launchAppAction(timestamp, file, section);
}

bool Dock::is3DWM()
{
    bool ret = false;
    if  (wmName.isEmpty())
        wmName = dbusHandler->getCurrentWM();

    if (wmName == "deepin wm")
        ret = true;

    return ret;
}

bool Dock::isWaylandEnv()
{
    return isWayland;
}

WindowInfoK *Dock::handleActiveWindowChangedK(uint activeWin)
{
    return waylandManager->handleActiveWindowChangedK(activeWin);
}

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

void Dock::saveDockedApps()
{
    // 保存驻留应用信息
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

void Dock::removeAppEntry(Entry *entry)
{
    if (entry) {
        entries->remove(entry);
    }
}

void Dock::handleWindowGeometryChanged()
{
    if (SETTING->getHideMode() == HideMode::SmartHide)
        return;

    updateHideState(false);
}

Entry *Dock::getEntryByWindowId(XWindow windowId)
{
    return entries->getByWindowId(windowId);
}

QString Dock::getDesktopFromWindowByBamf(XWindow windowId)
{
    return dbusHandler->getDesktopFromWindowByBamf(windowId);
}

void Dock::registerWindowWayland(const QString &objPath)
{
    return waylandManager->registerWindow(objPath);
}

void Dock::unRegisterWindowWayland(const QString &objPath)
{
    return waylandManager->unRegisterWindow(objPath);
}

AppInfo *Dock::identifyWindow(WindowInfoBase *winInfo, QString &innerId)
{
    return windowIdentify->identifyWindow(winInfo, innerId);
}

void Dock::markAppLaunched(AppInfo *appInfo)
{
    if (!appInfo || !appInfo->isValidApp())
        return;

    QString desktopFile = appInfo->getFileName();
    qInfo() << "markAppLaunched: desktopFile is " << desktopFile;
    dbusHandler->markAppLaunched(desktopFile);
}

void Dock::deleteWindow(XWindow xid)
{
    entries->deleteWindow(xid);
}

ForceQuitAppMode Dock::getForceQuitAppStatus()
{
    return forceQuitAppStatus;
}

QVector<QString> Dock::getWinIconPreferredApps()
{
    return SETTING->getWinIconPreferredApps();
}

void Dock::handleLauncherItemDeleted(QString itemPath)
{
    for (auto entry : entries->filterDockedEntries()) {
        if (entry->getFileName() == itemPath) {
            undockEntry(entry);
            break;
        }
    }
}

// 在收到 launcher item 更新的信号后，需要更新相关信息，包括 appInfo、innerId、名称、图标、菜单。
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

double Dock::getOpacity()
{
    return SETTING->getOpacity();
}

QRect Dock::getFrontendWindowRect()
{
    return frontendWindowRect;
}

int Dock::getDisplayMode()
{
    return int(SETTING->getDisplayMode());
}

void Dock::setDisplayMode(int mode)
{
    SETTING->setDisplayMode(DisplayMode(mode));
}

QStringList Dock::getDockedApps()
{
    return SETTING->getDockedApps();
}

QStringList Dock::getEntryPaths()
{
    QStringList ret;
    for (auto id : entries->getEntryIDs()) {
        ret.push_back(entryDBusObjPathPrefix + id);
    }

    return ret;
}

HideMode Dock::getHideMode()
{
    return SETTING->getHideMode();
}

void Dock::setHideMode(HideMode mode)
{
    SETTING->setHideMode(mode);
}

Dock::HideState Dock::getHideState()
{
    return hideState;
}

void Dock::setHideState(Dock::HideState state)
{
    hideState = state;
}

uint Dock::getHideTimeout()
{
    return SETTING->getHideTimeout();
}

void Dock::setHideTimeout(uint timeout)
{
    SETTING->setHideTimeout(timeout);
}

uint Dock::getIconSize()
{
    return SETTING->getIconSize();
}

void Dock::setIconSize(uint size)
{
    SETTING->setIconSize(size);
}

int Dock::getPosition()
{
    return int(SETTING->getPositionMode());
}

void Dock::setPosition(int position)
{
    SETTING->setPositionMode(PositonMode(position));
}

uint Dock::getShowTimeout()
{
    return SETTING->getShowTimeout();
}

void Dock::setShowTimeout(uint timeout)
{
    return SETTING->setShowTimeout(timeout);
}

uint Dock::getWindowSizeEfficient()
{
    return SETTING->getWindowSizeEfficient();
}

void Dock::setWindowSizeEfficient(uint size)
{
    SETTING->setWindowSizeEfficient(size);
}

uint Dock::getWindowSizeFashion()
{
    return SETTING->getWindowSizeFashion();
}

void Dock::setWindowSizeFashion(uint size)
{
    SETTING->setWindowSizeFashion(size);
}

