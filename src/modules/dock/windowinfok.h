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

#ifndef WINDOWINFOK_H
#define WINDOWINFOK_H

#include "windowinfobase.h"
#include "dbusplasmawindow.h"

#include <QString>

class Entry;
class ProcessInfo;

// wayland下窗口信息
class WindowInfoK: public WindowInfoBase
{
public:
    WindowInfoK(PlasmaWindow *window, XWindow _xid = 0);
    virtual ~WindowInfoK();

    virtual bool shouldSkip();
    virtual QString getIcon();
    virtual QString getTitle();
    virtual bool isDemandingAttention();
    virtual bool allowClose();
    virtual void close(uint32_t timestamp);
    virtual void activate();
    virtual void minimize();
    virtual bool isMinimized();
    virtual int64_t getCreatedTime();
    virtual QString getDisplayName();
    virtual QString getWindowType();
    virtual void update();
    virtual void killClient();

    QString getAppId();
    void setAppId(QString _appId);
    bool changeXid(XWindow _xid);
    PlasmaWindow *getPlasmaWindow();
    bool updateGeometry();
    void updateTitle();
    void updateDemandingAttention();
    void updateIcon();
    void updateAppId();
    void updateInternalId();
    void updateCloseable();
    void updateProcessInfo();

private:
    bool updateCalled;
    QString appId;
    uint32_t internalId;
    bool demaningAttention;
    bool closeable;
    bool minimized;
    PlasmaWindow *plasmaWindow;
    Rect geometry;
};

#endif // WINDOWINFOK_H
