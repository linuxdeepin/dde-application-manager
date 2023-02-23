// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COMMON_H
#define COMMON_H

#include <QString>

const QString AMServiceName = "/org/deepin/dde/Application1/Manager";
const QString autostartDir  = "autostart";
const QString proxychainsBinary = "proxychains4";

const QString configLauncher        = "com.deepin.dde.launcher";
const QString keyAppsUseProxy       = "Apps_Use_Proxy";
const QString keyAppsDisableScaling = "Apps_Disable_Scaling";

const QString KeyXGnomeAutostartDelay = "X-GNOME-Autostart-Delay";
const QString KeyXGnomeAutoRestart    = "X-GNOME-AutoRestart";
const QString KeyXDeepinCreatedBy     = "X-Deepin-CreatedBy";
const QString KeyXDeepinAppID         = "X-Deepin-AppID";

const QString configStartdde        = "com.deepin.dde.startdde";
const QString keyAutostartDelay = "Autostart_Delay";
const QString keyMemCheckerEnabled = "Memchecker_Enabled";
const QString keySwapSchedEnabled = "swap-sched-enabled";

const QString configXsettings = "com.deepin.xsettings";
const QString keyScaleFactor = "Scale_Factor";

const QString uiAppSchedHooksDir = "/usr/lib/UIAppSched.hooks";
const QString launchedHookDir    = uiAppSchedHooksDir + "/launched";

const QString cpuFreqAdjustFile   = "/usr/share/startdde/app_startup.conf";
const QString performanceGovernor = "performance";

const int restartRateLimitSeconds = 60;

const QString sysMemLimitConfig = "/usr/share/startdde/memchecker.json";

const int defaultMinMemAvail = 300;  // 300M
const int defaultMaxSwapUsed = 1200; // 1200M

const QString autostartAdded         = "added";
const QString autostartDeleted       = "deleted";

#endif // COMMON_H
