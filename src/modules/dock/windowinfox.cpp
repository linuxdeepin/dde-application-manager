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

#include "windowinfox.h"
#include "appinfo.h"
#include "xcbutils.h"
#include "dstring.h"
#include "common.h"
#include "processinfo.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QTimer>
#include <QImage>
#include <QIcon>

#define XCB XCBUtils::instance()

WindowInfoX::WindowInfoX(XWindow _xid)
 : WindowInfoBase ()
 , x(0)
 , y(0)
 , width(0)
 , height(0)
 , hasWMTransientFor(false)
 , hasXEmbedInfo(false)
 , updateCalled(false)
{
    xid = _xid;
    createdTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); // 获取当前时间，精确到纳秒 
}

WindowInfoX::~WindowInfoX()
{

}

bool WindowInfoX::shouldSkip()
{
    qInfo() << "window " << xid << " shouldSkip?";
    if (!updateCalled) {
        update();
        updateCalled = true;
    }

    if (hasWmStateSkipTaskBar() || isValidModal() || shouldSkipWithWMClass())
        return true;

    for (auto atom : wmWindowType) {
        if (atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_DIALOG") && !isActionMinimizeAllowed())
            return true;

        if (atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_UTILITY")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_COMBO")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_DESKTOP") // 桌面属性窗口
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_DND")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_DOCK")    // 任务栏属性窗口
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_DROPDOWN_MENU")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_MENU")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_NOTIFICATION")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_POPUP_MENU")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_SPLASH")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_TOOLBAR")
        || atom == XCB->getAtom("_NET_WM_WINDOW_TYPE_TOOLTIP"))
            return true;
    }

    return false;
}

QString WindowInfoX::getIcon()
{
    if (icon.isEmpty())
        icon = getIconFromWindow();

    return icon;
}

void WindowInfoX::activate()
{
    XCB->changeActiveWindow(xid);
    QTimer::singleShot(50, [&] {
        XCB->restackWindow(xid);
    });
}

void WindowInfoX::minimize()
{
    XCB->minimizeWindow(xid);
}

bool WindowInfoX::isMinimized()
{
    return containAtom(wmState, XCB->getAtom("_NET_WM_STATE_HIDDEN"));
}

int64_t WindowInfoX::getCreatedTime()
{
    return createdTime;
}

QString WindowInfoX::getWindowType()
{
    return "X11";
}

bool WindowInfoX::allowClose()
{
    // 允许关闭的条件：
    // 1. 不设置 functions 字段，即MotifHintFunctions 标志位；
    // 2. 或者设置了 functions 字段并且 设置了 MotifFunctionAll 标志位；
    // 3. 或者设置了 functions 字段并且 设置了 MotifFunctionClose 标志位。
    // 相关定义在 motif-2.3.8/lib/Xm/MwmUtil.h 。
    if ((motifWmHints.flags & MotifHintFunctions) == 0
     || (motifWmHints.functions & MotifFunctionAll) != 0
     || (motifWmHints.functions & MotifFunctionClose) != 0)
        return true;

    for (auto action : wmAllowedActions) {
        if (action == XCB->getAtom("_NET_WM_ACTION_CLOSE")) {
            return true;
        }
    }

    return false;
}

QString WindowInfoX::getDisplayName()
{
    XWindow winId = xid;
    //QString role = wmRole;
    QString className(wmClass.className.c_str());
    QString instance;
    if (wmClass.instanceName.size() > 0) {
        int pos = QString(wmClass.instanceName.c_str()).lastIndexOf('/');
        if (pos != -1)
            instance.remove(0, pos + 1);
    }
    qInfo() << "getDisplayName class:" << className << " ,instance:" << instance;

    //if (!role.isEmpty() && !className.isEmpty())
    //    return className + " " + role;

    if (!className.isEmpty())
        return className;

    if (!instance.isEmpty())
        return instance;


    QString _wmName = wmName;
    if (!_wmName.isEmpty()) {
        int pos = _wmName.lastIndexOf('-');
        if (pos != -1 && !_wmName.startsWith("-")) {
            _wmName.truncate(pos);
            return _wmName;
        }
    }

    if (processInfo) {
        QString exe {processInfo->getEnv("exe").c_str()};
        if (!exe.isEmpty())
            return exe;
    }

    return QString("window:%1").arg(winId);
}

void WindowInfoX::killClient()
{
    XCB->killClientChecked(xid);
}

QString WindowInfoX::uuid()
{
    return QString();
}

QString WindowInfoX::getGtkAppId()
{
    return gtkAppId;
}

QString WindowInfoX::getFlatpakAppId()
{
    return flatpakAppId;
}

QString WindowInfoX::getWmRole()
{
    return wmRole;
}

WMClass WindowInfoX::getWMClass()
{
    return wmClass;
}

QString WindowInfoX::getWMName()
{
    return wmName;
}

ConfigureEvent *WindowInfoX::getLastConfigureEvent()
{
    return lastConfigureNotifyEvent;
}

void WindowInfoX::setLastConfigureEvent(ConfigureEvent *event)
{
    lastConfigureNotifyEvent = event;
}

bool WindowInfoX::isGeometryChanged(int _x, int _y, int _width, int _height)
{
    return !(_x == x && _y == y && _width == width && _height == height);
}

void WindowInfoX::setGtkAppId(QString _gtkAppId)
{
    gtkAppId = _gtkAppId;
}

void WindowInfoX::updateMotifWmHints()
{
    // get from XCB
    motifWmHints = XCB->getWindowMotifWMHints(xid);
}

// XEmbed info
// 一般 tray icon 会带有 _XEMBED_INFO 属性
void WindowInfoX::updateHasXEmbedInfo()
{
    hasXEmbedInfo = XCB->hasXEmbedInfo(xid);
}

/**
 * @brief WindowInfoX::genInnerId 生成innerId
 * @param winInfo
 * @return
 */
QString WindowInfoX::genInnerId(WindowInfoX *winInfo)
{
    XWindow winId = winInfo->getXid();
    QString wmClassName, wmInstance;
    WMClass wmClass = winInfo->getWMClass();
    if (wmClass.className.size() > 0)
        wmClassName = wmClass.className.c_str();

    if (wmClass.instanceName.size() > 0) {
        QString instanceName(wmClass.instanceName.c_str());
        instanceName.remove(0, instanceName.lastIndexOf('/') + 1);
        wmInstance = instanceName;
    }

    QString exe, args;
    if (winInfo->getProcess()) {
        exe = winInfo->getProcess()->getExe().c_str();
        for (auto arg : winInfo->getProcess()->getArgs()) {
            QString argStr(arg.c_str());
            if (argStr.contains("/") || argStr == "." || argStr == "..") {
                args += "%F ";
            } else {
                args += argStr + " ";
            }
        }

        if (args.size() > 0)
            args.remove(args.size() - 2, 1);
    }

    bool hasPid = winInfo->getPid() != 0;
    QString str;
    // NOTE: 不要使用 wmRole，有些程序总会改变这个值比如 GVim
    if (wmInstance.isEmpty() && wmClassName.isEmpty() && exe.isEmpty() && winInfo->getGtkAppId().isEmpty()) {
        if (!winInfo->getWMName().isEmpty())
            str = QString("wmName:%1").arg(winInfo->getWMName());
        else
            str = QString("windowId:%1").arg(winInfo->getXid());
    } else {
        str = QString("wmInstance:%1,wmClass:%2,exe:%3,args:%4,hasPid:%5,gtkAppId:%6").arg(wmInstance).arg(wmClassName).arg(exe).arg(args).arg(hasPid).arg(winInfo->getGtkAppId());
    }

    QByteArray encryText = QCryptographicHash::hash(str.toLatin1(), QCryptographicHash::Md5);
    QString innerId = windowHashPrefix + encryText.toHex();
    qInfo() << "genInnerId window " << winId << " innerId :" << innerId;
    return innerId;
}

// 更新窗口类型
void WindowInfoX::updateWmWindowType()
{
    wmWindowType.clear();
    for (auto ty : XCB->getWMWindoType(xid)) {
        wmWindowType.push_back(ty);
    }
}

// 更新窗口许可动作
void WindowInfoX::updateWmAllowedActions()
{
    wmAllowedActions.clear();
    for (auto action : XCB->getWMAllowedActions(xid)) {
        wmAllowedActions.push_back(action);
    }
}

void WindowInfoX::updateWmState()
{
    wmState.clear();
    for (auto a : XCB->getWMState(xid)) {
        wmState.push_back(a);
    }
}

void WindowInfoX::updateWmClass()
{
    wmClass = XCB->getWMClass(xid);
}

void WindowInfoX::updateWmName()
{
    auto name = XCB->getWMName(xid);
    if (!name.empty())
        wmName = name.c_str();

    title = getTitle();
}

void WindowInfoX::updateIcon()
{
    icon = getIconFromWindow();
}

void WindowInfoX::updateHasWmTransientFor()
{
    if (XCB->getWMTransientFor(xid) == 1)
        hasWMTransientFor = true;
}

/**
 * @brief WindowInfoX::update 更新窗口信息（在识别窗口时执行一次）
 */
void WindowInfoX::update()
{
    updateWmClass();
    updateWmState();
    updateWmWindowType();
    updateWmAllowedActions();
    updateHasWmTransientFor();
    updateProcessInfo();
    updateWmName();
    innerId = genInnerId(this);
}

// TODO 从窗口中获取图标， 并设置best size   be used in Entry
QString WindowInfoX::getIconFromWindow()
{
    return QString();
}

bool WindowInfoX::isActionMinimizeAllowed()
{
    return containAtom(wmAllowedActions, XCB->getAtom("_NET_WM_ACTION_MINIMIZE"));
}

bool WindowInfoX::hasWmStateDemandsAttention()
{
    return containAtom(wmState, XCB->getAtom("_NET_WM_STATE_DEMANDS_ATTENTION"));
}

bool WindowInfoX::hasWmStateSkipTaskBar()
{
    return containAtom(wmState, XCB->getAtom("_NET_WM_STATE_SKIP_TASKBAR"));
}

bool WindowInfoX::hasWmStateModal()
{
    return containAtom(wmState, XCB->getAtom("_NET_WM_STATE_MODAL"));
}

bool WindowInfoX::isValidModal()
{
    return hasWmStateModal() && hasWmStateModal();
}

// 通过WMClass判断是否需要隐藏此窗口
bool WindowInfoX::shouldSkipWithWMClass()
{
    bool ret = false;
    if (wmClass.instanceName == "explorer.exe" && wmClass.className == "Wine")
        ret = true;
    else if (wmClass.className == "dde-launcher")
        ret = true;

    return ret;
}

void WindowInfoX::updateProcessInfo()
{
    XWindow winId = xid;
    pid = XCB->getWMPid(winId);
    if (processInfo)
        delete processInfo;

    qInfo() << "updateProcessInfo: pid=" << pid;
    processInfo = new ProcessInfo(pid);
    if (!processInfo->isValid()) {
        // try WM_COMMAND
        auto wmComand = XCB->getWMCommand(winId);
        if (wmComand.size() > 0) {
            delete processInfo;
            processInfo = new ProcessInfo(wmComand);
        }
    }

    qInfo() << "updateProcessInfo: pid is " << pid;
}

bool WindowInfoX::getUpdateCalled()
{
    return updateCalled;
}

void WindowInfoX::setInnerId(QString _innerId)
{
    innerId = _innerId;
}

QString WindowInfoX::getTitle()
{
    QString name = wmName;
    if (name.isEmpty())
        name = getDisplayName();

    return name;
}

bool WindowInfoX::isDemandingAttention()
{
    return hasWmStateDemandsAttention();
}

void WindowInfoX::close(uint32_t timestamp)
{
    XCB->requestCloseWindow(xid, timestamp);
}


