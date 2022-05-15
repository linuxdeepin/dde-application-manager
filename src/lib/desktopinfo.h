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

#ifndef DESKTOPINFO_H
#define DESKTOPINFO_H

#include "keyfile.h"

#include <string>
#include <vector>

const std::string MainSection        = "Desktop Entry";
const std::string KeyType            = "Type";
const std::string KeyVersion         = "Version";
const std::string KeyName            = "Name";
const std::string KeyGenericName     = "GenericName";
const std::string KeyNoDisplay       = "NoDisplay";
const std::string KeyComment         = "Comment";
const std::string KeyIcon            = "Icon";
const std::string KeyHidden          = "Hidden";
const std::string KeyOnlyShowIn      = "OnlyShowIn";
const std::string KeyNotShowIn       = "NotShowIn";
const std::string KeyTryExec         = "TryExec";
const std::string KeyExec            = "Exec";
const std::string KeyPath            = "Path";
const std::string KeyTerminal        = "Terminal";
const std::string KeyMimeType        = "MimeType";
const std::string KeyCategories      = "Categories";
const std::string KeyKeywords        = "Keywords";
const std::string KeyStartupNotify   = "StartupNotify";
const std::string KeyStartupWMClass  = "StartupWMClass";
const std::string KeyURL             = "URL";
const std::string KeyActions         = "Actions";
const std::string KeyDBusActivatable = "DBusActivatable";

const std::string TypeApplication = "Application";
const std::string TypeLink        = "Link";
const std::string TypeDirectory   = "Directory";

const std::string envDesktopEnv         = "XDG_CURRENT_DESKTOP";

typedef struct DesktopAction
{
    DesktopAction()
        : section("")
        , name("")
        , exec("")
    {
    }
    std::string section;
    std::string name;
    std::string exec;
} DesktopAction;

// 应用Desktop信息类
class DesktopInfo {
public:
    explicit DesktopInfo(const std::string &_fileName);
    ~DesktopInfo();

    std::string getFileName();
    std::string getExecutable();
    bool isValidDesktop();
    bool shouldShow();
    bool getNoDisplay();
    bool getIsHidden();
    bool getShowIn(std::vector<std::string> desktopEnvs);
    bool isExecutableOk();
    bool isInstalled();
    static bool isDesktopAction(std::string name);
    std::vector<DesktopAction> getActions();
    static DesktopInfo getDesktopInfoById(std::string appId);
    bool getTerminal();

    std::string getId();
    std::string getGenericName();
    std::string getName();
    std::string getIcon();
    std::string getCommandLine();
    std::vector<std::string> getKeywords();
    std::vector<std::string> getCategories();
    void setDesktopOverrideExec(const std::string &execStr);

    KeyFile *getKeyFile();

private:
    std::string getTryExec();
    bool findExecutable(std::string &exec);

    static std::vector<std::string> currentDesktops;

    std::string m_fileName;
    std::string m_id;
    std::string m_name;
    std::string m_icon;
    std::string m_overRideExec;
    bool m_isValid;
    KeyFile m_keyFile;
};

// 应用目录类
class AppsDir {
public:
    explicit AppsDir(const std::string &dirPath);
    ~AppsDir();

    std::string getPath();
    std::map<std::string, bool> getAppNames();
    static std::vector<DesktopInfo> getAllDesktopInfos();

private:
    std::string m_path;
    std::map<std::string, bool> m_appNames;
};

#endif // DESKTOPINFO_H
