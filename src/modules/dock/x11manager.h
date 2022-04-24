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

#ifndef X11MANAGER_H
#define X11MANAGER_H

#include "windowinfox.h"
#include "xcbutils.h"

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QTimer>

class Dock;

class X11Manager : public QObject
{
    Q_OBJECT
public:
    explicit X11Manager(Dock *_dock, QObject *parent = nullptr);

    WindowInfoX *findWindowByXid(XWindow xid);
    WindowInfoX *registerWindow(XWindow xid);
    void unregisterWindow(XWindow xid);

    void handleClientListChanged();
    void handleActiveWindowChangedX();
    void listenRootWindowXEvent();
    void listenWindowXEvent(WindowInfoX *winInfo);

    void handleRootWindowPropertyNotifyEvent(XCBAtom atom);
    void handleDestroyNotifyEvent(XWindow xid);
    void handleMapNotifyEvent(XWindow xid);
    void handleConfigureNotifyEvent(XWindow xid, int x, int y, int width, int height);
    void handlePropertyNotifyEvent(XWindow xid, XCBAtom atom);

    void eventHandler(uint8_t type, void *event);
    void listenWindowEvent(WindowInfoX *winInfo);
    void listenXEventUseXlib();
    void listenXEventUseXCB();

signals:
    void needUpdateHideState(bool delay);

private:
    void addWindowLastConfigureEvent(XWindow xid, ConfigureEvent* event);
    QPair<ConfigureEvent*, QTimer*> getWindowLastConfigureEvent(XWindow xid);
    void delWindowLastConfigureEvent(XWindow xid);

    QMap<XWindow, WindowInfoX *> windowInfoMap;
    Dock *dock;
    QMap<XWindow, QPair<ConfigureEvent*, QTimer*>> windowLastConfigureEventMap; // 手动回收ConfigureEvent和QTimer
    QMutex mutex;
    XWindow rootWindow; // 根窗口
    bool listenXEvent;  // 监听X事件
};

#endif // X11MANAGER_H
