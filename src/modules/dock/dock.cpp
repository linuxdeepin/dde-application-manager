// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
#include "impl/application_manager.h"

#include "macro.h"

#include <QDir>
#include <QMap>
#include <QTimer>

#define SETTING DockSettings::instance()
#define XCB XCBUtils::instance()

Dock::Dock(ApplicationManager *applicationManager, QObject *parent)
 : SynModule(parent)
 , m_entriesSum(0)
 , m_windowIdentify(new WindowIdentify(this))
 , m_entries(new Entries(this))
 , m_ddeLauncherVisible(false)
 , m_hideState(HideState::Unknown)
 , m_activeWindow(nullptr)
 , m_activeWindowOld(nullptr)
 , m_dbusHandler(new DBusHandler(this))
 , m_windowOperateMutex(QMutex(QMutex::NonRecursive))
 , m_showRecent(false)
 , m_showMultiWindow(false)
 , m_applicationManager(applicationManager)
{
    registeModule("dock");

    QString sessionType {getenv("XDG_SESSION_TYPE")};
    qInfo() << "sessionType=" << sessionType;
    if (sessionType.contains("wayland")) {
        // wayland env
        m_isWayland = true;
        m_waylandManager = new WaylandManager(this);
        m_dbusHandler->listenWaylandWMSignals();
    } else if (sessionType.contains("x11")) {
        // x11 env
        m_isWayland = false;
        m_x11Manager = new X11Manager(this);
    }

    initSettings();
    initEntries();

    // 初始化智能隐藏定时器
    m_smartHideTimer = new QTimer(this);
    m_smartHideTimer->setSingleShot(true);
    connect(m_smartHideTimer, &QTimer::timeout, this, &Dock::smartHideModeTimerExpired);

    if (!m_isWayland) {
        std::thread thread([&] {
            // Xlib方式
            m_x11Manager->listenXEventUseXlib();
            // XCB方式
            //listenXEventUseXCB();
        });
        thread.detach();
        m_x11Manager->listenRootWindowXEvent();
        connect(m_x11Manager, &X11Manager::requestUpdateHideState, this, &Dock::updateHideState);
        connect(m_x11Manager, &X11Manager::requestHandleActiveWindowChange, this, &Dock::handleActiveWindowChanged);
        connect(m_x11Manager, &X11Manager::requestAttachOrDetachWindow, this, &Dock::attachOrDetachWindow);
    }
}

Dock::~Dock()
{

}

/**
 * @brief Dock::dockEntry 驻留应用
 * @param entry 应用实例
 * @return
 */
bool Dock::dockEntry(Entry *entry, bool moveToEnd)
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
            // 在目标文件存在的情况下，先删除，防止出现驻留不成功的情况
            if (QFile::exists(newFile))
                QFile::remove(newFile);
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

    // 如果是最近打开应用，通过右键菜单的方式驻留，且当前是时尚模式，那么就让entry驻留到末尾
    if (moveToEnd && SETTING->getDisplayMode() == DisplayMode::Fashion)
        m_entries->moveEntryToLast(entry);

    entry->setIsDocked(true);
    entry->updateMenu();
    entry->updateMode();
    return true;
}

/**
 * @brief Dock::undockEntry 取消驻留
 * @param entry 应用实例
 */
void Dock::undockEntry(Entry *entry, bool moveToEnd)
{
    if (!entry->getIsDocked()) {
        qInfo() << "undockEntry: " << entry->getId() << " is not docked";
        // 当应用图标在最近打开区域的时候，此时该应用是未驻留的应用，如果该最近打开应用没有打开窗口，将这个图标
        // 拖动到回收站了，此时调用的是undock方法，根据需求，需要将该图标删除
        if (!entry->hasWindow()) {
            // 没有子窗口的情况下，从列表中移除
            removeAppEntry(entry);
            saveDockedApps();
        }
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
        QStringList suffixs {".desktop", ".sh", ".png"};
        for (auto &ext : suffixs) {
            QString fileName = filebase + ext;
            QFile file(fileName);
            if (file.exists()) {
                file.remove();
            }
        }
    }

    if (entry->hasWindow()) {
        // 移除驻留后，如果当前应用存在子窗口，那么会将移除最近使用应用中最后一个没有子窗口的窗口
        m_entries->removeLastRecent();
        if (desktopFile.contains(scratchDir) && entry->getCurrentWindowInfo()) {
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
                AppInfo *app = m_windowIdentify->identifyWindow(entry->getCurrentWindowInfo(), innerId);
                // TODO update entry's innerId
                entry->setApp(app);
                entry->setInnerId(innerId);
            }
        }
        // 如果存在窗口，在时尚模式下，就会移动到最近打开区域，此时让它移动到最后
        if (moveToEnd && SETTING->getDisplayMode() == DisplayMode::Fashion)
            m_entries->moveEntryToLast(entry);

        entry->updateIcon();
        entry->setIsDocked(false);
        entry->updateName();
        entry->updateMenu();
        // 更新模式， 是在应用区域还是在最近打开区域
        entry->updateMode();
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
    return QString("e%1T%2").arg(++m_entriesSum).arg(QString::number(QDateTime::currentSecsSinceEpoch(), 16));
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
        bool isReg = m_x11Manager->findWindowByXid(winId);
        bool isContainedInClientList = m_clientList.indexOf(winId) != -1;
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
    m_ddeLauncherVisible = visible;
}

/**
 * @brief Dock::getWMName 获取窗管名称
 * @return
 */
QString Dock::getWMName()
{
    return m_wmName;
}

/**
 * @brief Dock::setWMName 设置窗管名称
 * @param name 窗管名称
 */
void Dock::setWMName(QString name)
{
    m_wmName = name;
}

/**
 * @brief Dock::setSyncConfig 同步配置 TODO
 * @param ba 配置数据
 */
void Dock::setSyncConfig(QByteArray ba)
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
    return m_dbusHandler->createPlasmaWindow(objPath);
}

/**
 * @brief Dock::listenKWindowSignals
 * @param windowInfo
 */
void Dock::listenKWindowSignals(WindowInfoK *windowInfo)
{
    m_dbusHandler->listenKWindowSignals(windowInfo);
}

/**
 * @brief Dock::removePlasmaWindowHandler 关闭窗口后需求对应的connect
 * @param window
 */
void Dock::removePlasmaWindowHandler(PlasmaWindow *window)
{
    m_dbusHandler->removePlasmaWindowHandler(window);
}

/**
 * @brief Dock::presentWindows 显示窗口
 * @param windows 窗口id
 */
void Dock::presentWindows(QList<uint> windows)
{
    m_dbusHandler->presentWindows(windows);
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
    if (!m_activeWindow)
        return m_activeWindowOld;

    return m_activeWindow;

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
    return QList<XWindow>(m_clientList);
}

/**
 * @brief Dock::setClientList 设置窗口client列表
 */
void Dock::setClientList(QList<XWindow> value)
{
    m_clientList = value;
}

/**
 * @brief Dock::closeWindow 关闭窗口
 * @param windowId 窗口id
 */
void Dock::closeWindow(uint32_t windowId)
{
    qInfo() << "Close Window " << windowId;
    if (m_isWayland) {
        WindowInfoK *info = m_waylandManager->findWindowByXid(windowId);
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
    return m_entries->getEntryIDs();
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
    if (m_frontendWindowRect == QRect(x, y, width, height)) {
        qInfo() << "SetFrontendWindowRect: no changed";
        return;
    }

    m_frontendWindowRect.setX(x);
    m_frontendWindowRect.setY(y);
    m_frontendWindowRect.setWidth(width);
    m_frontendWindowRect.setHeight(height);
    updateHideState(false);

    Q_EMIT frontendWindowRectChanged(m_frontendWindowRect);
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

    Entry *entry = m_entries->getByInnerId(app->getInnerId());
    if (!entry)
        entry = new Entry(this, app, app->getInnerId());

    if (!dockEntry(entry))
        return false;

    entry->startExport();
    m_entries->insert(entry, index);

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

void Dock::setShowRecent(bool visible)
{
    if (visible == m_showRecent)
        return;

    SETTING->setShowRecent(visible);
    onShowRecentChanged(visible);
}

bool Dock::showRecent() const
{
    return m_showRecent;
}

void Dock::setShowMultiWindow(bool visible)
{
    if (m_showMultiWindow == visible)
        return;

    SETTING->setShowMultiWindow(visible);
    onShowMultiWindowChanged(visible);
}

bool Dock::showMultiWindow() const
{
    return m_showMultiWindow;
}

/**
 * @brief Dock::moveEntry 移动驻留程序顺序
 * @param oldIndex
 * @param newIndex
 */
void Dock::moveEntry(int oldIndex, int newIndex)
{
    m_entries->move(oldIndex, newIndex);
    saveDockedApps();
}

/**
 * @brief Dock::isOnDock 是否在任务栏
 * @param desktopFile desktopFile文件全路径
 * @return
 */
bool Dock::isOnDock(QString desktopFile)
{
    return m_entries->getByDesktopFilePath(desktopFile);
}

/**
 * @brief Dock::queryWindowIdentifyMethod 查询窗口识别方式
 * @param windowId 窗口id
 * @return
 */
QString Dock::queryWindowIdentifyMethod(XWindow windowId)
{
    return m_entries->queryWindowIdentifyMethod(windowId);
}

/**
 * @brief Dock::getDockedAppsDesktopFiles 获取驻留应用desktop文件
 * @return
 */
QStringList Dock::getDockedAppsDesktopFiles()
{
    QStringList ret;
    for (auto entry: m_entries->filterDockedEntries()) {
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
    m_forceQuitAppStatus = SETTING->getForceQuitAppMode();
    connect(SETTING, &DockSettings::hideModeChanged, this, [ this ](HideMode mode) {
        this->updateHideState(false);
    });
    connect(SETTING, &DockSettings::displayModeChanged, this, [](DisplayMode mode) {
       qInfo() << "display mode change to " << static_cast<int>(mode);
    });
    connect(SETTING, &DockSettings::positionModeChanged, this, [](PositionMode mode) {
       qInfo() << "position mode change to " << static_cast<int>(mode);
    });
    connect(SETTING, &DockSettings::forceQuitAppChanged, this, [ this ](ForceQuitAppMode mode) {
       qInfo() << "forceQuitApp change to " << int(mode);
       m_forceQuitAppStatus = mode;
       m_entries->updateEntriesMenu();
    });
    connect(SETTING, &DockSettings::showRecentChanged, this, &Dock::onShowRecentChanged);
    connect(SETTING, &DockSettings::showMultiWindowChanged, this, &Dock::onShowMultiWindowChanged);
}

/**
 * @brief Dock::initEntries 初始化应用
 */
void Dock::initEntries()
{
    loadAppInfos();
    initClientList();
}

/**
 * @brief Dock::loadAppInfos 加载本地驻留和最近使用的应用信息
 */
void Dock::loadAppInfos()
{
    // 读取是否显示最近打开应用
    m_showRecent = SETTING->showRecent();
    // 读取是否显示多开窗口拆分
    m_showMultiWindow = SETTING->showMultiWindow();
    // 初始化驻留应用信息和最近使用的应用的信息
    auto loadApps = [ this ](const QStringList &apps, bool isDocked) {
        for (const QString &app : apps) {
            QString path = app;
            DesktopInfo info(path.toStdString());
            if (!info.isValidDesktop())
                continue;

            AppInfo *appInfo = new AppInfo(info);
            Entry *entryObj = new Entry(this, appInfo, appInfo->getInnerId());
            entryObj->setIsDocked(isDocked);
            entryObj->updateMode();
            entryObj->updateMenu();
            entryObj->startExport();
            m_entries->append(entryObj);
        }
    };

    loadApps(SETTING->getDockedApps(), true);
    QStringList recentApps = SETTING->getRecentApps();
    if (recentApps.size() > MAX_UNOPEN_RECENT_COUNT) {
        QStringList tempApps = recentApps;
        recentApps.clear();
        for (int i = 0; i < MAX_UNOPEN_RECENT_COUNT; i++)
            recentApps << tempApps[i];
    }
    loadApps(recentApps, false);
    saveDockedApps();
}

/**
 * @brief Dock::initClientList 初始化窗口列表，关联到对应应用
 */
void Dock::initClientList()
{
    if (m_isWayland) {
        m_dbusHandler->loadClientList();
    } else {
        QList<XWindow> clients;
        for (auto c : XCB->instance()->getClientList())
            clients.push_back(c);

        // 依次注册窗口
        qSort(clients.begin(), clients.end());
        m_clientList = clients;
        for (auto winId : m_clientList) {
            WindowInfoX *winInfo = m_x11Manager->registerWindow(winId);
            attachOrDetachWindow(static_cast<WindowInfoBase *>(winInfo));
        }
    }
}

/**
 * @brief Dock::findWindowByXidX 通过id获取窗口信息
 * @param xid
 * @return
 */
WindowInfoX *Dock::findWindowByXidX(XWindow xid)
{
    return m_x11Manager->findWindowByXid(xid);
}

/**
 * @brief Dock::findWindowByXidK 通过xid获取窗口  TODO wayland和x11下窗口尽量完全剥离， 不应该存在通过xid查询wayland窗口的情况
 * @param xid
 * @return
 */
WindowInfoK *Dock::findWindowByXidK(XWindow xid)
{
    return m_waylandManager->findWindowByXid(xid);
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
    return hasInterSectionX(winRect, m_frontendWindowRect);
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

    return hasInterSectionK(rect, m_frontendWindowRect);
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

    if (position == int(PositionMode::Left) || position == int(PositionMode::Right))
        return ltX <= rbX && ltY < rbY;

    if (position == int(PositionMode::Top) || position == int(PositionMode::Bottom))
        return ltX < rbX && ltY <= rbY;

    return ltX < rbX && ltY < rbY;
}

/**
 * @brief Dock::getDockedEntryByDesktopFile 获取应用实例
 * @param desktopFile desktopFile文件全路径
 * @return
 */
Entry *Dock::getDockedEntryByDesktopFile(const QString &desktopFile)
{
    return m_entries->getDockedEntryByDesktopFile(desktopFile);
}

/**
 * @brief Dock::shouldHideOnSmartHideMode 判断智能隐藏模式下当前任务栏是否应该隐藏
 * @return
 */
bool Dock::shouldHideOnSmartHideMode()
{
    if (!m_activeWindow || m_ddeLauncherVisible)
        return false;

    if (!m_isWayland) {
        XWindow activeWinId = m_activeWindow->getXid();

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
    }

    return isWindowDockOverlapK(m_activeWindow);
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
    if (m_ddeLauncherVisible) {
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
        m_smartHideTimer->start(delay ? smartHideTimerDelay : 0);
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

    if (state != m_hideState) {
        m_hideState = state;
        Q_EMIT hideStateChanged(static_cast<int>(m_hideState));
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
    QMutexLocker locker(&m_windowOperateMutex);
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
            AppInfo *appInfo = m_windowIdentify->identifyWindow(info, innerId);
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

    // 在新增窗口后，同步最近打开应用到com.deepin.dde.dock.json的DConfig配置文件中
    updateRecentApps();
}

/**
 * @brief Dock::attachWindow 关联窗口
 * @param info 窗口信息
 */
void Dock::attachWindow(WindowInfoBase *info)
{
    // TODO: entries中存在innerid为空的entry， 导致后续新应用通过innerid获取应用一直能获取到
    Entry *entry = m_entries->getByInnerId(info->getEntryInnerId());
    if (entry) {
        // entry existed
        entry->attachWindow(info);
    } else {
        m_entries->removeLastRecent();
        entry = new Entry(this, info->getAppInfo(), info->getEntryInnerId());
        if (entry->attachWindow(info)) {
            entry->startExport();
            m_entries->append(entry);
        }
    }
}

/**
 * @brief Dock::detachWindow 分离窗口
 * @param info 窗口信息
 */
void Dock::detachWindow(WindowInfoBase *info)
{
    Entry *entry = m_entries->getByWindowId(info->getXid());
    if (!entry)
        return;

    if (entry->detachWindow(info))
        removeEntryFromDock(entry);
}

/**
 * @brief Dock::launchApp 启动应用
 * @param timestamp 时间
 * @param files 应用打开文件
 */
void Dock::launchApp(const QString desktopFile, uint32_t timestamp, QStringList files)
{
    m_applicationManager->LaunchApp(desktopFile, timestamp, files, false);
}

/**
 * @brief Dock::launchAppAction 启动应用响应
 * @param timestamp
 * @param file
 * @param section
 */
void Dock::launchAppAction(const QString desktopFile, QString action, uint32_t timestamp)
{
    m_applicationManager->LaunchAppAction(desktopFile, action ,timestamp, false);
}

/**
 * @brief Dock::is3DWM 当前窗口模式 2D/3D
 * @return
 */
bool Dock::is3DWM()
{
    bool ret = false;
    if  (m_wmName.isEmpty())
        m_wmName = m_dbusHandler->getCurrentWM();

    if (m_wmName == "deepin wm")
        ret = true;

    return ret;
}

/**
 * @brief Dock::isWaylandEnv 当前环境
 * @return
 */
bool Dock::isWaylandEnv()
{
    return m_isWayland;
}

/**
 * @brief Dock::handleActiveWindowChangedK 处理活动窗口改变事件 wayland环境
 * @param activeWin
 * @return
 */
WindowInfoK *Dock::handleActiveWindowChangedK(uint activeWin)
{
    return m_waylandManager->findWindowById(activeWin);
}

/**
 * @brief Dock::handleActiveWindowChanged 处理活动窗口改变事件 X11环境
 * @param info
 */
void Dock::handleActiveWindowChanged(WindowInfoBase *info)
{
    qInfo() << "handleActiveWindowChanged";
    if (!info) {
        m_activeWindowOld = info;
        m_activeWindow = nullptr;
        return;
    }

    m_activeWindow = info;
    XWindow winId = m_activeWindow->getXid();
    m_entries->handleActiveWindowChanged(winId);
    updateHideState(true);
}

/**
 * @brief Dock::saveDockedApps 保存驻留应用信息
 */
void Dock::saveDockedApps()
{
    QStringList dockedApps;
    for (auto entry : m_entries->filterDockedEntries()) {
        QString path = entry->getApp()->getFileName();
        dockedApps << path;
    }

    SETTING->setDockedApps(dockedApps);

    // 在驻留任务栏的时候，同时更新最近打开应用的信息
    updateRecentApps();
}

void Dock::updateRecentApps()
{
    QStringList unDockedApps;
    QList<Entry *> recentEntrys = m_entries->unDockedEntries();
    for (Entry *entry : recentEntrys) {
        QString path = entry->getApp()->getFileName();
        unDockedApps << path;
    }

    // 保存未驻留的应用作为最近打开的应用
    SETTING->setRecentApps(unDockedApps);
}

void Dock::removeEntryFromDock(Entry *entry)
{
    // 如果是最近打开应用
    if (m_entries->shouldInRecent()) {
        // 更新entry的导出窗口信息
        entry->updateExportWindowInfos();
        // 更新entry的右键菜单的信息
        entry->updateMenu();
        // 更新entry的当前窗口的信息
        entry->setCurrentWindowInfo(nullptr);
        // 移除应用后，同时更新最近打开的应用
        updateRecentApps();
        // 如果是高效模式，则发送消息或者关闭了显示最近应用的功能，则从任务栏移除
        if ((SETTING->getDisplayMode() == DisplayMode::Efficient
                || !m_showRecent) && !entry->getIsDocked()) {
            Q_EMIT entryRemoved(entry->getId());
        }
    } else {
        removeAppEntry(entry);
        // 移除应用后，同时更新最近打开的应用
        updateRecentApps();
    }
}

void Dock::onShowRecentChanged(bool visible)
{
    if (m_showRecent == visible)
        return;

    m_showRecent = visible;
    m_entries->updateShowRecent();
    Q_EMIT showRecentChanged(visible);
}

void Dock::onShowMultiWindowChanged(bool visible)
{
    if (m_showMultiWindow == visible)
        return;

    m_showMultiWindow = visible;
    Q_EMIT showMultiWindowChanged(visible);
}

/** 移除应用实例
 * @brief Dock::removeAppEntry
 * @param entry
 */
void Dock::removeAppEntry(Entry *entry)
{
    if (entry) {
        m_entries->remove(entry);
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
    return m_entries->getByWindowId(windowId);
}

/**
 * @brief Dock::getDesktopFromWindowByBamf 通过bamf软件服务获取指定窗口的desktop文件
 * @param windowId
 * @return
 */
QString Dock::getDesktopFromWindowByBamf(XWindow windowId)
{
    return m_dbusHandler->getDesktopFromWindowByBamf(windowId);
}

/**
 * @brief Dock::registerWindowWayland 注册wayland窗口
 * @param objPath
 */
void Dock::registerWindowWayland(const QString &objPath)
{
    return m_waylandManager->registerWindow(objPath);
}

/**
 * @brief Dock::unRegisterWindowWayland 取消注册wayland窗口
 * @param objPath
 */
void Dock::unRegisterWindowWayland(const QString &objPath)
{
    return m_waylandManager->unRegisterWindow(objPath);
}

/**
 * @brief Dock::isShowingDesktop
 * @return
 */
bool Dock::isShowingDesktop()
{
    return m_dbusHandler->wlShowingDesktop();
}

/**
 * @brief Dock::identifyWindow 识别窗口
 * @param winInfo
 * @param innerId
 * @return
 */
AppInfo *Dock::identifyWindow(WindowInfoBase *winInfo, QString &innerId)
{
    return m_windowIdentify->identifyWindow(winInfo, innerId);
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
    m_dbusHandler->markAppLaunched(desktopFile);
}

/**
 * @brief Dock::getForceQuitAppStatus 获取强制关闭应用状态
 * @return
 */
ForceQuitAppMode Dock::getForceQuitAppStatus()
{
    return m_forceQuitAppStatus;
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
    for (auto entry : m_entries->filterDockedEntries()) {
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
    Entry *entry = m_entries->getByDesktopFilePath(itemPath);
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
    return m_frontendWindowRect;
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
    DisplayMode displayMode = static_cast<DisplayMode>(mode);
    SETTING->setDisplayMode(displayMode);
    m_entries->setDisplayMode(displayMode);
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
    for (auto id : m_entries->getEntryIDs()) {
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
    return m_hideState;
}

/**
 * @brief Dock::setHideState 设置任务栏隐藏状态
 * @param state
 */
void Dock::setHideState(HideState state)
{
    m_hideState = state;
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
