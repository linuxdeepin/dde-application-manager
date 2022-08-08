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

#include "dbushandler.h"
#include "dock.h"
#include "windowinfok.h"
#include "dbusbamfapplication.h"

DBusHandler::DBusHandler(Dock *_dock, QObject *parent)
    : QObject(parent)
    , dock(_dock)
    , session(QDBusConnection::sessionBus())
    , launcherEnd(new LauncherBackEnd("org.deepin.dde.daemon.Launcher1", "/org/deepin/dde/daemon/Launcher1", session, this))
    , launcherFront(new LauncherFront("org.deepin.dde.Launcher1", "/org/deepin/dde/Launcher1", session, this))
    , wm(new com::deepin::WM("com.deepin.wm", "/com/deepin/wm", session, this))
    , wmSwitcher(new com::deepin::WMSwitcher("com.deepin.wmWMSwitcher", "/com/deepin/WMSwitcher", session, this))
    , kwaylandManager(nullptr)
{
    // 关联org.deepin.dde.daemon.Launcher1事件 ItemChanged
    connect(launcherEnd, &LauncherBackEnd::ItemChanged, this, &DBusHandler::handleLauncherItemChanged);

    // 关联org.deepin.dde.Launcher1事件 VisibleChanged
    connect(launcherFront, &LauncherFront::VisibleChanged, this, [&](bool visible) {
        dock->setDdeLauncherVisible(visible);
        dock->updateHideState(false);
    });

    // 关联com.deepin.WMSwitcher事件 WMChanged
    connect(wmSwitcher, &__WMSwitcher::WMChanged, this, [&](QString name) {dock->setWMName(name);});
}

// 关联com.deepin.daemon.KWayland.WindowManager事件
void DBusHandler::listenWaylandWMSignals()
{
    kwaylandManager = new com::deepin::daemon::kwayland::WindowManager("com.deepin.daemon.KWayland", "/com/deepin/daemon/KWayland/WindowManager", session, this);

    // ActiveWindowchanged
    connect(kwaylandManager, &__KwaylandManager::ActiveWindowChanged, this, &DBusHandler::handleWlActiveWindowChange);
    // WindowCreated
    connect(kwaylandManager, &__KwaylandManager::WindowCreated, this, [&] (const QString &ObjPath) {
        dock->registerWindowWayland(ObjPath);
    });
    // WindowRemove
    connect(kwaylandManager, &__KwaylandManager::WindowRemove, this, [&] (const QString &ObjPath) {
        dock->unRegisterWindowWayland(ObjPath);
    });
}

void DBusHandler::loadClientList()
{
    if (!kwaylandManager)
        return;

    // 加载已存在的窗口
    QDBusPendingReply<QVariantList> windowList = kwaylandManager->Windows();
    QVariantList windows = windowList.value();
    for (QVariant windowPath : windows)
        dock->registerWindowWayland(windowPath.toString());
}

void DBusHandler::handleLauncherItemChanged(const QString &status, LauncherItemInfo itemInfo, qlonglong categoryID)
{
    qInfo() << "handleLauncherItemChanged status:" << status << " Name:" << itemInfo.name << " ID:" << itemInfo.id;
    if (status == "deleted") {
        dock->handleLauncherItemDeleted(itemInfo.path);
    } else if (status == "created") {
        // don't need to download to dock when app reinstall
    } else if (status == "updated") {
        dock->handleLauncherItemUpdated(itemInfo.path);
    }
}

QString DBusHandler::getCurrentWM()
{
    return wmSwitcher->CurrentWM().value();
}

// TODO 扩展ApplicationManager Launch接口，允许带参数启动应用，暂时调用StartManager接口
void DBusHandler::launchApp(QString desktopFile, uint32_t timestamp, QStringList files)
{
    QDBusInterface interface = QDBusInterface("com.deepin.daemon.Display", "/com/deepin/StartManager", "com.deepin.StartManager");
    interface.call("LaunchApp", desktopFile, timestamp, files);
}

void DBusHandler::launchAppAction(QString desktopFile, QString action, uint32_t timestamp)
{
    QDBusInterface interface = QDBusInterface("com.deepin.daemon.Display", "/com/deepin/StartManager", "com.deepin.StartManager");
    interface.call("LaunchAppAction", desktopFile, action, timestamp);
}

void DBusHandler::markAppLaunched(const QString &filePath)
{
    QDBusInterface interface = QDBusInterface("org.deepin.daemon.AlRecorder1", "/org/deepin/daemon/AlRecorder1", "org.deepin.daemon.AlRecorder1");
    interface.call("MarkLaunched", filePath);
}

bool DBusHandler::wlShowingDesktop()
{
    bool ret = false;
    if (kwaylandManager)
        ret = kwaylandManager->IsShowingDesktop().value();

    return ret;
}

uint DBusHandler::wlActiveWindow()
{
    uint ret = 0;
    if (kwaylandManager)
        ret = kwaylandManager->ActiveWindow().value();

    return ret;
}

void DBusHandler::handleWlActiveWindowChange()
{
    uint activeWinInternalId = wlActiveWindow();
    if (activeWinInternalId == 0)
        return;

    WindowInfoK *info = dock->handleActiveWindowChangedK(activeWinInternalId);
    if (info && info->getXid() != 0) {
        WindowInfoBase *base = static_cast<WindowInfoBase *>(info);
        if (base) {
            dock->handleActiveWindowChanged(base);
        }
    } else {
        dock->updateHideState(false);
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
        auto entry = dock->getEntryByWindowId(windowInfo->getXid());
        if (!entry)
            return;

        if (entry->getCurrentWindowInfo() == windowInfo)
            entry->updateName();

        entry->updateExportWindowInfos();
    });

    // Icon changed
    connect(window, &PlasmaWindow::IconChanged, this, [=] {
        windowInfo->updateIcon();
        auto entry = dock->getEntryByWindowId(windowInfo->getXid());
        if (!entry)
            return;

        entry->updateIcon();
    });

    // DemandingAttention changed
    connect(window, &PlasmaWindow::DemandsAttentionChanged, this, [=] {
        windowInfo->updateDemandingAttention();
        auto entry = dock->getEntryByWindowId(windowInfo->getXid());
        if (!entry)
            return;

        entry->updateExportWindowInfos();
    });

    // Geometry changed
    connect(window, &PlasmaWindow::GeometryChanged, this, [=] {
        if (!windowInfo->updateGeometry())
            return;

        dock->handleWindowGeometryChanged();
    });
}

PlasmaWindow *DBusHandler::createPlasmaWindow(QString objPath)
{
    return new PlasmaWindow("com.deepin.daemon.KWayland", objPath, session, this);
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
    wm->PresentWindows(windows);
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

