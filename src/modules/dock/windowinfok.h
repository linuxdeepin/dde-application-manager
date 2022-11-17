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

#ifndef WINDOWINFOK_H
#define WINDOWINFOK_H

#include "windowinfobase.h"
#include "dbusplasmawindow.h"

#include <QString>

using PlasmaWindow = org::deepin::dde::kwayland1::PlasmaWindow;

class Entry;
class ProcessInfo;

// wayland下窗口信息
class WindowInfoK: public WindowInfoBase
{
public:
    explicit WindowInfoK(PlasmaWindow *window, XWindow _xid = 0);
    virtual ~WindowInfoK() override;

    virtual bool shouldSkip() override;
    virtual QString getIcon() override;
    virtual QString getTitle() override;
    virtual bool isDemandingAttention() override;
    virtual bool allowClose() override;
    virtual void close(uint32_t timestamp) override;
    virtual void activate() override;
    virtual void minimize() override;
    virtual bool isMinimized() override;
    virtual int64_t getCreatedTime() override;
    virtual QString getDisplayName() override;
    virtual QString getWindowType() override;
    virtual void update() override;
    virtual void killClient() override;
    virtual QString uuid() override;
    QString getInnerId() override;

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
    DockRect getGeometry();

private:
    bool m_updateCalled;
    QString m_appId;
    uint32_t m_internalId;
    bool m_demaningAttention;
    bool m_closeable;
    PlasmaWindow *m_plasmaWindow;
    DockRect m_geometry;
};

#endif // WINDOWINFOK_H
