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

#ifndef WINDOWIDENTIFY_H
#define WINDOWIDENTIFY_H

#include "windowpatterns.h"
#include "windowinfok.h"
#include "windowinfox.h"

#include <QObject>
#include <QVector>
#include <QMap>

class AppInfo;
class Dock;

typedef AppInfo *(*IdentifyFunc)(Dock *, WindowInfoX*, QString &innerId);

// 应用窗口识别类
class WindowIdentify : public QObject
{
    Q_OBJECT

public:
    explicit WindowIdentify(Dock *_dock, QObject *parent = nullptr);

    AppInfo *identifyWindow(WindowInfoBase *winInfo, QString &innerId);
    AppInfo *identifyWindowX11(WindowInfoX *winInfo, QString &innerId);
    AppInfo *identifyWindowWayland(WindowInfoK *winInfo, QString &innerId);

    static AppInfo *identifyWindowAndroid(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByPidEnv(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByCmdlineTurboBooster(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByCmdlineXWalk(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByFlatpakAppID(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByCrxId(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByRule(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByBamf(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByPid(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByScratch(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByGtkAppId(Dock *_dock, WindowInfoX *winInfo, QString &innerId);
    static AppInfo *identifyWindowByWmClass(Dock *_dock, WindowInfoX *winInfo, QString &innerId);

public Q_SLOTS:

private:
    AppInfo *fixAutostartAppInfo(QString fileName);
    static int32_t getAndroidUengineId(XWindow winId);
    static QString getAndroidUengineName(XWindow winId);

    Dock *dock;
    QMap<QString, IdentifyFunc> identifyWindowFuns;
};

#endif // IDENTIFYWINDOW_H
