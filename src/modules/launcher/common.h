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

#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QMultiMap>

// DBus服务、路径名
const QString dbusService = "org.deepin.dde.daemon.Launcher1";
const QString dbusPath = "/org/deepin/dde/daemon/Launcher1";

// 组策略配置
const QString configLauncher        = "com.deepin.dde.launcher";
const QString keyDisplayMode        = "Display_Mode";
const QString keyFullscreen         = "Fullscreen";
const QString keyAppsUseProxy       = "Apps_Use_Proxy";
const QString keyAppsDisableScaling = "Apps_Disable_Scaling";
const QString keyAppsHidden         = "Apps_Hidden";
const QString keyPackageNameSearch  = "Search_Package_Name";

// 应用配置
const QString lastoreDataDir = "/var/lib/lastore";
const QString desktopPkgMapFile = lastoreDataDir + "/desktop_package.json";
const QString applicationsFile  = lastoreDataDir + "/applications.json";

const QString ddeDataDir = "/usr/share/dde/data/";
const QString appNameTranslationsFile = ddeDataDir + "app_name_translations.json";

// 应用状态
const QString appStatusCreated  = "created";
const QString appStatusModified = "updated";
const QString appStatusDeleted  = "deleted";

// flatpak应用
const QString flatpakBin = "flatpak";

// 启动器执行程序
const QString launcherExe = "/usr/bin/dde-launcher";
#endif // COMMON_H
