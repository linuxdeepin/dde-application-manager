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

#ifndef WAYLANDMANAGER_H
#define WAYLANDMANAGER_H

#include "windowinfok.h"

#include <QObject>
#include <QMap>
#include <QMutex>

class Dock;

// 管理wayland窗口
class WaylandManager : public QObject
{
    Q_OBJECT
public:
    explicit WaylandManager(Dock *_dock, QObject *parent = nullptr);

    void registerWindow(const QString &objPath);
    void unRegisterWindow(const QString &objPath);

    WindowInfoK *handleActiveWindowChangedK(uint activeWin);
    WindowInfoK *findWindowByXid(XWindow xid);
    WindowInfoK *findWindowByObjPath(QString objPath);
    void insertWindow(QString objPath, WindowInfoK *windowInfo);
    void deleteWindow(QString objPath);

private:
    Dock *dock;
    QMap<QString, WindowInfoK *> kWinInfos; // dbusObjectPath -> kwayland window Info
    QMap<XWindow, WindowInfoK *> windowInfoMap;
    QMutex mutex;
};

#endif // WAYLANDMANAGER_H
