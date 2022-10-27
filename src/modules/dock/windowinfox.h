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
    virtual ~WindowInfoX() override;

    virtual bool shouldSkip() override;
    virtual QString getIcon() override;
    virtual QString getTitle() override;
    virtual bool isDemandingAttention() override;
    virtual void close(uint32_t timestamp) override;
    virtual void activate() override;
    virtual void minimize() override;
    virtual bool isMinimized() override;
    virtual int64_t getCreatedTime() override;
    virtual QString getDisplayName() override;
    virtual QString getWindowType() override;
    virtual bool allowClose() override;
    virtual void update() override;
    virtual void killClient() override;
    virtual QString uuid() override;

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

private:
    int16_t m_x;
    int16_t m_y;
    uint16_t m_width;
    uint16_t m_height;
    QVector<XCBAtom> m_wmState;
    QVector<XCBAtom> m_wmWindowType;
    QVector<XCBAtom> m_wmAllowedActions;
    bool m_hasWMTransientFor;
    WMClass m_wmClass;
    QString m_wmName;
    bool m_hasXEmbedInfo;

    // 自定义atom属性
    QString m_gtkAppId;
    QString m_flatpakAppId;
    QString m_wmRole;
    MotifWMHints m_motifWmHints;

    bool m_updateCalled;
    ConfigureEvent *m_lastConfigureNotifyEvent;
};

#endif // WINDOWINFOX_H
