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

#include "x11manager.h"
#include "dock.h"
#include "common.h"

#include <QDebug>
#include <QTimer>

/*
 *  使用Xlib监听X Events
 *  使用XCB接口与X进行交互
 * */

#include <ctype.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#define XCB XCBUtils::instance()

X11Manager::X11Manager(Dock *_dock, QObject *parent)
 : QObject(parent)
 , dock(_dock)
 , mutex(QMutex(QMutex::NonRecursive))
 , listenXEvent(true)
{
    rootWindow = XCB->getRootWindow();
}

void X11Manager::listenXEventUseXlib()
{

    Display *dpy;
    int screen;
    char *displayname = nullptr;
    Window w;
    XSetWindowAttributes attr;
    XWindowAttributes wattr;

    dpy = XOpenDisplay (displayname);
    if (!dpy) {
        exit (1);
     }

     screen = DefaultScreen (dpy);
     w = RootWindow(dpy, screen);

     const struct {
         const char *name;
         long mask;
      } events[] = {
         { "keyboard", KeyPressMask | KeyReleaseMask | KeymapStateMask },
         { "mouse", ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
                  LeaveWindowMask | PointerMotionMask | Button1MotionMask |
                  Button2MotionMask | Button3MotionMask | Button4MotionMask |
                  Button5MotionMask | ButtonMotionMask },
         { "button", ButtonPressMask | ButtonReleaseMask },
         { "expose", ExposureMask },
         { "visibility", VisibilityChangeMask },
         { "structure", StructureNotifyMask },
         { "substructure", SubstructureNotifyMask | SubstructureRedirectMask },
         { "focus", FocusChangeMask },
         { "property", PropertyChangeMask },
         { "colormap", ColormapChangeMask },
         { "owner_grab_button", OwnerGrabButtonMask },
         { nullptr, 0 }
    };

    long mask = 0;
    for (int i = 0; events[i].name; i++)
    mask |= events[i].mask;

    attr.event_mask = mask;

    XGetWindowAttributes(dpy, w, &wattr);

    attr.event_mask &= ~SubstructureRedirectMask;
    XSelectInput(dpy, w, attr.event_mask);

    while (listenXEvent) {
        XEvent event;
        XNextEvent (dpy, &event);

        if (event.type == DestroyNotify) {
            XDestroyWindowEvent *eD = (XDestroyWindowEvent *) (&event);
            //qInfo() <<  "DestroyNotify windowId=" << eD->window;

            handleDestroyNotifyEvent(XWindow(eD->window));
        } else if (event.type == MapNotify) {
            XMapEvent *eM = (XMapEvent *)(&event);
            //qInfo() << "MapNotify windowId=" << eM->window;

            handleMapNotifyEvent(XWindow(eM->window));
        } else if (event.type == ConfigureNotify ) {
            XConfigureEvent *eC = (XConfigureEvent *) (&event);
            //qInfo() << "ConfigureNotify windowId=" << eC->window;

            handleConfigureNotifyEvent(XWindow(eC->window), eC->x, eC->y, eC->width, eC->height);
        } else if (event.type == PropertyNotify) {
            XPropertyEvent *eP = (XPropertyEvent *) (&event);
            //qInfo() << "PropertyNotify windowId=" << eP->window;

            handlePropertyNotifyEvent(XWindow(eP->window), XCBAtom(eP->atom));
        } else {
            //qInfo() << "Unknown event type " << event.type;
        }

    }

    XCloseDisplay (dpy);
}

void X11Manager::listenXEventUseXCB()
{
    /*
    xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(XCB->getConnect(), XCB->getRootWindow());
    xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(XCB->getConnect(), cookie, NULL);
    if (reply) {
        uint32_t valueMask = reply->your_event_mask;
        valueMask &= ~XCB_CW_OVERRIDE_REDIRECT;
        uint32_t mask[2] = {0};
        mask[0] = valueMask;
        //xcb_change_window_attributes(XCB->getConnect(), XCB->getRootWindow(), valueMask, mask);

        free(reply);
    }

    xcb_generic_event_t *event;
    while ( (event = xcb_wait_for_event (XCB->getConnect())) ) {
        eventHandler(event->response_type & ~0x80, event);
    }
    */
}

// 注册X11窗口
WindowInfoX *X11Manager::registerWindow(XWindow xid)
{
    qInfo() << "registWindow: windowId=" << xid;
    WindowInfoX *ret = nullptr;
    do {
        if (windowInfoMap.find(xid) != windowInfoMap.end()) {
            ret = windowInfoMap[xid];
            break;
        }

        WindowInfoX *winInfo = new WindowInfoX(xid);
        if (!winInfo)
            break;

        listenWindowXEvent(winInfo);
        windowInfoMap[xid] = winInfo;
        ret = winInfo;
    } while (0);

    return ret;
}

// 取消注册X11窗口
void X11Manager::unregisterWindow(XWindow xid)
{
    qInfo() << "unregisterWindow: windowId=" << xid;
    if (windowInfoMap.find(xid) != windowInfoMap.end()) {
        windowInfoMap.remove(xid);
    }
}

WindowInfoX *X11Manager::findWindowByXid(XWindow xid)
{
    WindowInfoX *ret = nullptr;
    if (windowInfoMap.find(xid) != windowInfoMap.end())
        ret = windowInfoMap[xid];

    return ret;
}

void X11Manager::handleClientListChanged()
{
    QSet<XWindow> newClientList, oldClientList, addClientList, rmClientList;
    for (auto atom : XCB->getClientList())
        newClientList.insert(atom);

    for (auto atom : dock->getClientList())
        oldClientList.insert(atom);

    addClientList = newClientList - oldClientList;
    rmClientList = oldClientList - newClientList;

    for (auto xid : addClientList) {
        WindowInfoX *info = registerWindow(xid);
        if (!XCB->isGoodWindow(xid))
            continue;

        uint32_t pid = XCB->getWMPid(xid);
        WMClass wmClass = XCB->getWMClass(xid);
        QString wmName(XCB->getWMName(xid).c_str());
        if (pid != 0 || (wmClass.className.size() > 0 && wmClass.instanceName.size() > 0)
                || wmName.size() > 0 || XCB->getWMCommand(xid).size() > 0)
            dock->attachWindow(info);
    }

    for (auto xid : rmClientList) {
        WindowInfoX *info = windowInfoMap[xid];
        if (info) {
            dock->detachWindow(info);
            unregisterWindow(xid);
        } else {
            // no window
            auto entry = dock->getEntryByWindowId(xid);
            if (entry && !dock->isDocked(entry->getFileName())) {
                dock->removeAppEntry(entry);
            }
        }
    }
}

void X11Manager::handleActiveWindowChangedX()
{
    XWindow active = XCB->getActiveWindow();
    WindowInfoX *info = findWindowByXid(active);
    dock->handleActiveWindowChanged(info);
}

void X11Manager::listenRootWindowXEvent()
{
    uint32_t eventMask = EventMask::XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
    XCB->registerEvents(rootWindow, eventMask);
    handleActiveWindowChangedX();
    handleClientListChanged();
}

void X11Manager::listenWindowXEvent(WindowInfoX *winInfo)
{
    uint32_t eventMask = EventMask::XCB_EVENT_MASK_PROPERTY_CHANGE | EventMask::XCB_EVENT_MASK_STRUCTURE_NOTIFY | EventMask::XCB_EVENT_MASK_VISIBILITY_CHANGE;
    XCB->registerEvents(winInfo->getXid(), eventMask);
}

void X11Manager::handleRootWindowPropertyNotifyEvent(XCBAtom atom)
{
    if (atom == XCB->getAtom("_NET_CLIENT_LIST"))
        handleClientListChanged();
    else if (atom == XCB->getAtom("_NET_ACTIVE_WINDOW"))
        handleActiveWindowChangedX();
    else if (atom == XCB->getAtom("_NET_SHOWING_DESKTOP"))
        Q_EMIT requestUpdateHideState(false);
}

// destory event
void X11Manager::handleDestroyNotifyEvent(XWindow xid)
{
    WindowInfoX *winInfo = findWindowByXid(xid);
    if (!winInfo)
        return;

    dock->detachWindow(winInfo);
    unregisterWindow(xid);
}

// map event
void X11Manager::handleMapNotifyEvent(XWindow xid)
{
    WindowInfoX *winInfo = registerWindow(xid);
    if (!winInfo)
        return;

    // TODO QTimer不能在非主线程执行，使用单独线程开发定时器处理非主线程类似定时任务
    //QTimer::singleShot(2 * 1000, this, [=] {
        qInfo() << "handleMapNotifyEvent: pass 2s, now call idnetifyWindow, windowId=" << winInfo->getXid();
        QString innerId;
        AppInfo *appInfo = dock->identifyWindow(winInfo, innerId);
        dock->markAppLaunched(appInfo);
    //});
}

// config changed event 检测窗口大小调整和重绘应用
void X11Manager::handleConfigureNotifyEvent(XWindow xid, int x, int y, int width, int height)
{
    WindowInfoX *winInfo = findWindowByXid(xid);
    if (!winInfo || dock->getDockHideMode() != HideMode::SmartHide)
        return;

    WMClass wmClass = winInfo->getWMClass();
    if (wmClass.className.c_str() == frontendWindowWmClass)
        return;     // ignore frontend window ConfigureNotify event

    Q_EMIT requestUpdateHideState(winInfo->isGeometryChanged(x, y, width, height));
}

// property changed event
void X11Manager::handlePropertyNotifyEvent(XWindow xid, XCBAtom atom)
{
    if (xid == rootWindow) {
        handleRootWindowPropertyNotifyEvent(atom);
        return;
    }

    WindowInfoX *winInfo = findWindowByXid(xid);
    if (!winInfo)
        return;

    QString newInnerId;
    bool needAttachOrDetach = false;
    if (atom == XCB->getAtom("_NET_WM_STATE")) {
        winInfo->updateWmState();
        needAttachOrDetach = true;
    } else if (atom == XCB->getAtom("_GTK_APPLICATION_ID")) {
        QString gtkAppId;
        winInfo->setGtkAppId(gtkAppId);
        newInnerId = winInfo->genInnerId(winInfo);
    } else if (atom == XCB->getAtom("_NET_WM_PID")) {
        winInfo->updateProcessInfo();
        newInnerId = winInfo->genInnerId(winInfo);
    } else if (atom == XCB->getAtom("_NET_WM_NAME")) {
        winInfo->updateWmName();
        newInnerId = winInfo->genInnerId(winInfo);
    } else if (atom == XCB->getAtom("_NET_WM_ICON")) {
        winInfo->updateIcon();
    } else if (atom == XCB->getAtom("_NET_WM_ALLOWED_ACTIONS")) {
        winInfo->updateWmAllowedActions();
    } else if (atom == XCB->getAtom("_MOTIF_WM_HINTS")) {
        winInfo->updateMotifWmHints();
    } else if (atom == XCB_ATOM_WM_CLASS) {
        winInfo->updateWmClass();
        newInnerId = winInfo->genInnerId(winInfo);
        needAttachOrDetach = true;
    } else if (atom == XCB->getAtom("_XEMBED_INFO")) {
        winInfo->updateHasXEmbedInfo();
        needAttachOrDetach = true;
    } else if (atom == XCB->getAtom("_NET_WM_WINDOW_TYPE")) {
        winInfo->updateWmWindowType();
        needAttachOrDetach = true;
    } else if (atom == XCB_ATOM_WM_TRANSIENT_FOR) {
        winInfo->updateHasWmTransientFor();
        needAttachOrDetach = true;
    }

    if (!newInnerId.isEmpty() && winInfo->getUpdateCalled() && winInfo->getInnerId() != newInnerId) {
        // winInfo.innerId changed
        dock->detachWindow(winInfo);
        winInfo->setInnerId(newInnerId);
        needAttachOrDetach = true;
    }

    if (needAttachOrDetach)
        dock->attachWindow(winInfo);

    Entry *entry = dock->getEntryByWindowId(xid);
    if (!entry)
        return;

    if (atom == XCB->getAtom("_NET_WM_STATE")) {
        entry->updateExportWindowInfos();
    } else if (atom == XCB->getAtom("_NET_WM_ICON")) {
        if (entry->getCurrentWindowInfo() == winInfo) {
            entry->updateIcon();
        }
    } else if (atom == XCB->getAtom("_NET_WM_NAME")) {
        if (entry->getCurrentWindowInfo() == winInfo) {
            entry->updateName();
        }
        entry->updateExportWindowInfos();
    } else if (atom == XCB->getAtom("_NET_WM_ALLOWED_ACTIONS")) {
        entry->updateMenu();
    }
}

void X11Manager::eventHandler(uint8_t type, void *event)
{
    qInfo() << "eventHandler" << "type = " << type;
    switch (type) {
    case XCB_MAP_NOTIFY:    // 17   注册新窗口
        qInfo() << "eventHandler: XCB_MAP_NOTIFY";
        break;
    case XCB_DESTROY_NOTIFY:    // 19   销毁窗口
        qInfo() << "eventHandler: XCB_DESTROY_NOTIFY";
        break;
    case XCB_CONFIGURE_NOTIFY:  // 22   窗口变化
        qInfo() << "eventHandler: XCB_CONFIGURE_NOTIFY";
        break;
    case XCB_PROPERTY_NOTIFY:   // 28   窗口属性改变
        qInfo() << "eventHandler: XCB_PROPERTY_NOTIFY";
        break;
    }
}

void X11Manager::addWindowLastConfigureEvent(XWindow xid, ConfigureEvent *event)
{
    delWindowLastConfigureEvent(xid);

    QMutexLocker locker(&mutex);
    QTimer *timer = new QTimer();
    timer->setInterval(configureNotifyDelay);
    windowLastConfigureEventMap[xid] = QPair(event, timer);
}

QPair<ConfigureEvent *, QTimer *> X11Manager::getWindowLastConfigureEvent(XWindow xid)
{
    QPair<ConfigureEvent *, QTimer *> ret;
    QMutexLocker locker(&mutex);
    if (windowLastConfigureEventMap.find(xid) != windowLastConfigureEventMap.end())
         ret = windowLastConfigureEventMap[xid];

    return ret;
}

void X11Manager::delWindowLastConfigureEvent(XWindow xid)
{
    QMutexLocker locker(&mutex);
    if (windowLastConfigureEventMap.find(xid) != windowLastConfigureEventMap.end()) {
        QPair<ConfigureEvent*, QTimer*> item = windowLastConfigureEventMap[xid];
        windowLastConfigureEventMap.remove(xid);
        delete item.first;
        item.second->deleteLater();
    }
}




