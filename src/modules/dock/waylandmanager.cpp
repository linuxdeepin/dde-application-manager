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

#include "waylandmanager.h"
#include "dock.h"
#include "xcbutils.h"

#define XCB XCBUtils::instance()

WaylandManager::WaylandManager(Dock *_dock, QObject *parent)
 : QObject(parent)
 , dock(_dock)
 , mutex(QMutex(QMutex::NonRecursive))
{

}


// 注册窗口
void WaylandManager::registerWindow(const QString &objPath)
{
    qInfo() << "registerWindow: " << objPath;
    if (findWindowByObjPath(objPath))
        return;

    PlasmaWindow *plasmaWindow = dock->createPlasmaWindow(objPath);
    if (!plasmaWindow) {
        qWarning() << "registerWindowWayland: createPlasmaWindow failed";
        return;
    }

    QString appId = plasmaWindow->AppId();
    QStringList list {"dde-dock", "dde-launcher", "dde-clipboard", "dde-osd", "dde-polkit-agent", "dde-simple-egl", "dmcs"};
    if (list.indexOf(appId))
        return;

    XWindow winId = XCB->allocId();     // XCB中未发现释放XID接口
    XWindow realId = plasmaWindow->WindowId();
    if (!realId)
        winId = realId;

    WindowInfoK *winInfo = new WindowInfoK(plasmaWindow, winId);
    dock->listenKWindowSignals(winInfo);
    insertWindow(objPath, winInfo);
    dock->attachOrDetachWindow(winInfo);
    if (realId) {
        windowInfoMap[realId] = winInfo;
    }
}

// 取消注册窗口
void WaylandManager::unRegisterWindow(const QString &objPath)
{
    qInfo() << "unRegisterWindow: " << objPath;
    WindowInfoK *winInfo = findWindowByObjPath(objPath);
    if (!winInfo)
        return;

    dock->removePlasmaWindowHandler(winInfo->getPlasmaWindow());
    dock->detachWindow(winInfo);
    deleteWindow(objPath);
}

WindowInfoK *WaylandManager::handleActiveWindowChangedK(uint activeWin)
{
    WindowInfoK *winInfo = nullptr;
    QMutexLocker locker(&mutex);
    for (auto iter = kWinInfos.begin(); iter != kWinInfos.end(); iter++) {
        if (iter.value()->getInnerId() == activeWin) {
            winInfo = iter.value();
            break;
        }
    }

    return winInfo;
}

WindowInfoK *WaylandManager::findWindowByXid(XWindow xid)
{
    WindowInfoK *winInfo = nullptr;
    for (auto iter = kWinInfos.begin(); iter != kWinInfos.end(); iter++) {
        if (iter.value()->getXid() == xid) {
            winInfo = iter.value();
            break;
        }
    }

    return winInfo;
}

WindowInfoK *WaylandManager::findWindowByObjPath(QString objPath)
{
    if (kWinInfos.find(objPath) == kWinInfos.end())
        return nullptr;

    return kWinInfos[objPath];
}

void WaylandManager::insertWindow(QString objPath, WindowInfoK *windowInfo)
{
    QMutexLocker locker(&mutex);
    kWinInfos[objPath] = windowInfo;
}

void WaylandManager::deleteWindow(QString objPath)
{
    kWinInfos.remove(objPath);
}

