// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbushandler.h"
#include "dock.h"
#include "windowinfok.h"
#include "dbusbamfapplication.h"

DBusHandler::DBusHandler(Dock *_dock, QObject *parent)
    : QObject(parent)
    , m_dock(_dock)
    , m_session(QDBusConnection::sessionBus())
    , m_launcherEnd(new LauncherBackEnd("org.deepin.dde.daemon.Launcher1", "/org/deepin/dde/daemon/Launcher1", m_session, this))
    , m_launcherFront(new LauncherFront("org.deepin.dde.Launcher1", "/org/deepin/dde/Launcher1", m_session, this))
    , m_wm(new com::deepin::WM("com.deepin.wm", "/com/deepin/wm", m_session, this))
    , m_wmSwitcher(new org::deepin::dde::WMSwitcher1("org.deepin.dde.WMSwitcher1", "/org/deepin/dde/WMSwitcher1", m_session, this))
    , m_kwaylandManager(nullptr)
    , m_xEventMonitor(nullptr)
{
    // 关联org.deepin.dde.daemon.Launcher1事件 ItemChanged
    connect(m_launcherEnd, &LauncherBackEnd::ItemChanged, this, &DBusHandler::handleLauncherItemChanged);

    // 关联org.deepin.dde.Launcher1事件 VisibleChanged
    connect(m_launcherFront, &LauncherFront::VisibleChanged, this, [&](bool visible) {
        m_dock->setDdeLauncherVisible(visible);
        m_dock->updateHideState(false);
    });

    // 关联org.deepin.dde.WMSwitcher1事件 WMChanged
    connect(m_wmSwitcher, &__WMSwitcher::WMChanged, this, [&](QString name) {m_dock->setWMName(name);});

    if (QString(getenv("XDG_SESSION_TYPE")).contains("wayland")) {
        m_xEventMonitor = new org::deepin::dde::XEventMonitor1("org.deepin.dde.XEventMonitor1", "/org/deepin/dde/XEventMonitor1", m_session, this);
        // 注册XEventMonitor区域为整个屏幕的区域
        m_activeWindowMonitorKey = m_xEventMonitor->RegisterFullScreen();
        // 关联XEventMonitor的ButtonRelease事件
        connect(m_xEventMonitor, &org::deepin::dde::XEventMonitor1::ButtonRelease, this, &DBusHandler::onActiveWindowButtonRelease);
    }
}

// 关联org.deepin.dde.KWayland.WindowManager事件
void DBusHandler::listenWaylandWMSignals()
{
    m_kwaylandManager = new org::deepin::dde::kwayland1::WindowManager("org.deepin.dde.KWayland1", "/org/deepin/dde/KWayland1/WindowManager", m_session, this);

    // ActiveWindowchanged
    connect(m_kwaylandManager, &__KwaylandManager::ActiveWindowChanged, this, &DBusHandler::handleWlActiveWindowChange);
    // WindowCreated
    connect(m_kwaylandManager, &__KwaylandManager::WindowCreated, this, [&] (const QString &ObjPath) {
        m_dock->registerWindowWayland(ObjPath);
    });
    // WindowRemove
    connect(m_kwaylandManager, &__KwaylandManager::WindowRemove, this, [&] (const QString &ObjPath) {
        m_dock->unRegisterWindowWayland(ObjPath);
    });
}

void DBusHandler::loadClientList()
{
    if (!m_kwaylandManager)
        return;

    // 加载已存在的窗口
    QDBusPendingReply<QVariantList> windowList = m_kwaylandManager->Windows();
    QVariantList windows = windowList.value();
    for (QVariant windowPath : windows)
        m_dock->registerWindowWayland(windowPath.toString());
}

void DBusHandler::handleLauncherItemChanged(const QString &status, LauncherItemInfo itemInfo, qlonglong categoryID)
{
    qInfo() << "handleLauncherItemChanged status:" << status << " Name:" << itemInfo.name << " ID:" << itemInfo.id;
    if (status == "deleted") {
        m_dock->handleLauncherItemDeleted(itemInfo.path);
    } else if (status == "created") {
        // don't need to download to dock when app reinstall
    } else if (status == "updated") {
        m_dock->handleLauncherItemUpdated(itemInfo.path);
    }
}

QString DBusHandler::getCurrentWM()
{
    return m_wmSwitcher->CurrentWM().value();
}

// TODO 扩展Application Manager Launch接口，允许带参数启动应用，暂时调用StartManager接口
void DBusHandler::launchApp(QString desktopFile, uint32_t timestamp, QStringList files)
{
    QDBusInterface interface = QDBusInterface("org.deepin.dde.Application1.Manager", "/org/deepin/dde/Application1/Manager", "org.deepin.dde.Application1.Manager");
    interface.call("LaunchApp", desktopFile, timestamp, files);
}

void DBusHandler::launchAppAction(QString desktopFile, QString action, uint32_t timestamp)
{
    QDBusInterface interface = QDBusInterface("org.deepin.dde.Application1.Manager", "/org/deepin/dde/Application1/Manager", "org.deepin.dde.Application1.Manager");
    interface.call("LaunchAppAction", desktopFile, action, timestamp);
}

void DBusHandler::markAppLaunched(const QString &filePath)
{
    QDBusInterface interface = QDBusInterface("org.deepin.dde.AlRecorder1", "/org/deepin/dde/AlRecorder1", "org.deepin.dde.AlRecorder1");
    interface.call("MarkLaunched", filePath);
}

bool DBusHandler::wlShowingDesktop()
{
    bool ret = false;
    if (m_kwaylandManager)
        ret = m_kwaylandManager->IsShowingDesktop().value();

    return ret;
}

uint DBusHandler::wlActiveWindow()
{
    uint ret = 0;
    if (m_kwaylandManager)
        ret = m_kwaylandManager->ActiveWindow().value();

    return ret;
}

void DBusHandler::handleWlActiveWindowChange()
{
    uint activeWinInternalId = wlActiveWindow();
    if (activeWinInternalId == 0)
        return;

    WindowInfoK *info = m_dock->handleActiveWindowChangedK(activeWinInternalId);
    if (info && info->getXid() != 0) {
        m_dock->handleActiveWindowChanged(info);
    } else {
        m_dock->updateHideState(false);
    }
}

void DBusHandler::onActiveWindowButtonRelease(int type, int x, int y, const QString &key)
{
    // 当鼠标松开区域事件的时候，取消注册，同时调用激活窗口的方法来触发智能隐藏的相关信号
    if (key != m_activeWindowMonitorKey)
        return;

    uint activeWinInternalId = wlActiveWindow();
    if (activeWinInternalId == 0)
        return;

    WindowInfoK *info = m_dock->handleActiveWindowChangedK(activeWinInternalId);
    if (!info)
        return;

    // 如果是在当前激活窗口区域内释放的，则触发检测智能隐藏的方法
    DockRect dockRect = info->getGeometry();
    if (dockRect.X <= x && x <= dockRect.X + dockRect.Width && dockRect.Y <= y && y <= dockRect.Y + dockRect.Height) {
        // 取消智能隐藏
        m_dock->updateHideState(false);
    }
}

void DBusHandler::listenKWindowSignals(WindowInfoK *windowInfo)
{
    PlasmaWindow *window = windowInfo->getPlasmaWindow();
    if (!window)
        return;

    // Title changed
    connect(window, &PlasmaWindow::TitleChanged, this, [=] {
        windowInfo->updateTitle();
        auto entry = m_dock->getEntryByWindowId(windowInfo->getXid());
        if (!entry)
            return;

        if (entry->getCurrentWindowInfo() == windowInfo)
            entry->updateName();

        entry->updateExportWindowInfos();
    });

    // Icon changed
    connect(window, &PlasmaWindow::IconChanged, this, [=] {
        windowInfo->updateIcon();
        auto entry = m_dock->getEntryByWindowId(windowInfo->getXid());
        if (!entry)
            return;

        entry->updateIcon();
    });

    // DemandingAttention changed
    connect(window, &PlasmaWindow::DemandsAttentionChanged, this, [=] {
        windowInfo->updateDemandingAttention();
        auto entry = m_dock->getEntryByWindowId(windowInfo->getXid());
        if (!entry)
            return;

        entry->updateExportWindowInfos();
    });

    // Geometry changed
    connect(window, &PlasmaWindow::GeometryChanged, this, [=] {
        if (!windowInfo->updateGeometry())
            return;

        m_dock->handleWindowGeometryChanged();
    });
}

PlasmaWindow *DBusHandler::createPlasmaWindow(QString objPath)
{
    return new PlasmaWindow("org.deepin.dde.KWayland1", objPath, m_session, this);
}

/**
 * @brief DBusHandler::removePlasmaWindowHandler 取消关联信号 TODO
 * @param window
 */
void DBusHandler::removePlasmaWindowHandler(PlasmaWindow *window)
{

}

void DBusHandler::presentWindows(QList<uint> windows)
{
    m_wm->PresentWindows(windows);
}

// TODO: 待优化点， 查看Bamf根据windowId获取对应应用desktopFile路径实现方式, 移除bamf依赖
QString DBusHandler::getDesktopFromWindowByBamf(XWindow windowId)
{
    QDBusInterface interface0 = QDBusInterface("org.ayatana.bamf", "/org/ayatana/bamf/matcher", "org.ayatana.bamf.matcher");
    QDBusReply<QString> replyApplication = interface0.call("ApplicationForXid", windowId);
    QString appObjPath = replyApplication.value();
    if (!replyApplication.isValid() || appObjPath.isEmpty())
        return "";


    QDBusInterface interface = QDBusInterface("org.ayatana.bamf", appObjPath, "org.ayatana.bamf.application");
    QDBusReply<QString> replyDesktopFile = interface.call("DesktopFile");

    if (replyDesktopFile.isValid())
        return replyDesktopFile.value();


    return "";
}
