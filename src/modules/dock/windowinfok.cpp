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

#include "windowinfok.h"
#include "entry.h"
#include "processinfo.h"
#include "appinfo.h"

#include <chrono>

WindowInfoK::WindowInfoK(PlasmaWindow *window, XWindow _xid)
 : WindowInfoBase ()
 , updateCalled(false)
 , internalId(0)
 , demaningAttention(false)
 , closeable(true)
 , minimized(true)
 , plasmaWindow(window)
{
    xid = _xid;
    createdTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); // 获取当前时间，精确到纳秒
}

WindowInfoK::~WindowInfoK()
{

}

bool WindowInfoK::shouldSkip()
{
    if (!updateCalled) {
        update();
        updateCalled = true;
    }

    bool skip = plasmaWindow->SkipTaskbar();

    // 添加窗口能否最小化判断, 如果窗口不能最小化则隐藏任务栏图标
    bool canMinimize = false;
    canMinimize = plasmaWindow->IsMinimizeable();
    if (!canMinimize)
        skip = true;

    if (skip) {
        // 白名单, 过滤类似“欢迎应用”， 没有最小化窗口但是需要在任务栏显示图标
        QStringList list { "dde-introduction"};
        if (list.indexOf(appId) != -1)
            skip = false;
    }

    return skip;
}

QString WindowInfoK::getIcon()
{
    return icon;
}

QString WindowInfoK::getTitle()
{
    return title;
}

bool WindowInfoK::isDemandingAttention()
{
    return demaningAttention;
}

bool WindowInfoK::allowClose()
{
    return closeable;
}

void WindowInfoK::close(uint32_t timestamp)
{
    plasmaWindow->RequestClose();
}

QString WindowInfoK::getAppId()
{
    return appId;
}

void WindowInfoK::setAppId(QString _appId)
{
    appId = _appId;
}

void WindowInfoK::activate()
{
    plasmaWindow->RequestActivate();
}

void WindowInfoK::minimize()
{
    plasmaWindow->RequestToggleMinimized();
}

bool WindowInfoK::isMinimized()
{
    return minimized;
}

bool WindowInfoK::changeXid(XWindow _xid)
{
    xid = _xid;
    return true;
}

PlasmaWindow *WindowInfoK::getPlasmaWindow()
{
    return plasmaWindow;
}

bool WindowInfoK::updateGeometry()
{
    DockRect rect = plasmaWindow->Geometry();
    if (geometry == rect)
        return false;

    geometry = rect;
    return true;
}

void WindowInfoK::updateTitle()
{
    title = plasmaWindow->Title();
}

void WindowInfoK::updateDemandingAttention()
{
    demaningAttention = plasmaWindow->IsDemandingAttention();
}

void WindowInfoK::updateIcon()
{
    icon = plasmaWindow->Icon();
}

void WindowInfoK::updateAppId()
{
    appId = plasmaWindow->AppId();
}

void WindowInfoK::updateInternalId()
{
    internalId = plasmaWindow->InternalId();
}

void WindowInfoK::updateCloseable()
{
    closeable = plasmaWindow->IsCloseable();
}

void WindowInfoK::updateProcessInfo()
{
    pid = plasmaWindow->Pid();
    processInfo = new ProcessInfo(pid);
}

/**
 * @brief WindowInfoK::getGeometry 获取窗口大小
 * @return
 */
DockRect WindowInfoK::getGeometry()
{
    return geometry;
}

int64_t WindowInfoK::getCreatedTime()
{
    return createdTime;
}

// 主要是为兼容X11
QString WindowInfoK::getDisplayName()
{
    return "";
}

QString WindowInfoK::getWindowType()
{
    return "Wayland";
}

void WindowInfoK::update()
{
    updateInternalId();
    updateAppId();
    updateIcon();
    updateTitle();
    updateGeometry();
    updateDemandingAttention();
    updateCloseable();
    updateProcessInfo();
}

void WindowInfoK::killClient()
{
}

