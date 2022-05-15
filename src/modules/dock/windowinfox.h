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

#ifndef WINDOWINFOX_H
#define WINDOWINFOX_H

#include "windowinfobase.h"
#include "xcbutils.h"

#include <QVector>

class AppInfo;

// X11下窗口信息 在明确X11环境下使用
class WindowInfoX: public WindowInfoBase
{
public:
    WindowInfoX(XWindow _xid = 0);
    virtual ~WindowInfoX();

    virtual bool shouldSkip();
    virtual QString getIcon();
    virtual QString getTitle();
    virtual bool isDemandingAttention();
    virtual void close(uint32_t timestamp);
    virtual void activate();
    virtual void minimize();
    virtual bool isMinimized();
    virtual int64_t getCreatedTime();
    virtual QString getDisplayName();
    virtual QString getWindowType();
    virtual bool allowClose();
    virtual void update();
    virtual void killClient();

    QString genInnerId(WindowInfoX *winInfo);
    QString getGtkAppId();
    QString getFlatpakAppId();
    QString getWmRole();
    WMClass getWMClass();
    QString getWMName();
    void updateProcessInfo();
    bool getUpdateCalled();
    void setInnerId(QString _innerId);
    ConfigureEvent *getLastConfigureEvent();
    void setLastConfigureEvent(ConfigureEvent *event);
    bool isGeometryChanged(int _x, int _y, int _width, int _height);
    void setGtkAppId(QString _gtkAppId);

    /************************更新XCB窗口属性*********************/
    void updateWmWindowType();
    void updateWmAllowedActions();
    void updateWmState();
    void updateWmClass();
    void updateMotifWmHints();
    void updateWmName();
    void updateIcon();
    void updateHasXEmbedInfo();
    void updateHasWmTransientFor();

private:
    QString getIconFromWindow();
    bool isActionMinimizeAllowed();
    bool hasWmStateDemandsAttention();
    bool hasWmStateSkipTaskBar();
    bool hasWmStateModal();
    bool isValidModal();
    bool shouldSkipWithWMClass();

    int16_t x, y;
    uint16_t width, height;
    QVector<XCBAtom> wmState;
    QVector<XCBAtom> wmWindowType;
    QVector<XCBAtom> wmAllowedActions;
    bool hasWMTransientFor;
    WMClass wmClass;
    QString wmName;
    bool hasXEmbedInfo;

    // 自定义atom属性
    QString gtkAppId;
    QString flatpakAppId;
    QString wmRole;
    MotifWMHints motifWmHints;

    bool updateCalled;
    ConfigureEvent *lastConfigureNotifyEvent;
};

#endif // WINDOWINFOX_H
