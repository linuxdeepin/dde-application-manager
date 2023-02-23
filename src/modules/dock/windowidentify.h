// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

private:
    AppInfo *fixAutostartAppInfo(QString fileName);
    static int32_t getAndroidUengineId(XWindow winId);
    static QString getAndroidUengineName(XWindow winId);

private:
    Dock *m_dock;
    QMap<QString, IdentifyFunc> m_identifyWindowFuns;
};

#endif // IDENTIFYWINDOW_H
