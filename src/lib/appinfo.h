// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPINFO_H
#define APPINFO_H

#include "keyfile.h"

#include <vector>
#include <memory>

class AppInfoManger
{
public:
    AppInfoManger();
    std::vector<std::string> getStringList(std::string session, std::string key);
    std::string toJson();
    std::string getFileName();
    static std::shared_ptr<AppInfoManger> loadByDesktopId(std::string desktopId);
    static std::string getDefaultApp(std::string mimeType, bool supportUri);
    static std::vector<std::string> getAppList(std::string mimeType);
    static bool setDefaultApp(std::string mimeType, std::string desktopId);
    static std::vector<std::string>& getXdgDataDirs();
    static std::vector<std::string>& getXdgAppsDirs();
    static std::vector<std::shared_ptr<AppInfoManger>> getAll(std::map<std::string, std::vector<std::string>> skipDirs);
    std::string getDesktopId();
    std::string getAppName();
    std::string getDisplayName();
    std::string getComment();
    std::string getIcon();
    std::string getCmdline();
    bool getCanDelete();
    void setCanDelete(bool bCanDelete);
    std::vector<std::string> getCategories();
    void setDesktopId(std::string desktopId);
    static std::string toJson(std::vector<std::shared_ptr<AppInfoManger>> appInfos);

private:
    static std::shared_ptr<AppInfoManger> initAppInfoByFile(std::string fileName);
    static std::string getAppName(KeyFile& keyFile);
    static std::string getIconByKeyFile(KeyFile& keyFile);
    static std::string getDesktopIdFile(std::string fileName);
    bool shouldShow();
private:
    std::string     desktopId;
    std::string     appName;
    std::string     displayName;
    std::string     comment;
    std::string     icon;
    std::string     cmdline;
    bool            canDelete;
    std::string     fileName;
    KeyFile         keyFile;
    static std::vector<std::string> xdgDataDirs;
    static std::vector<std::string> xdgAppDirs;
    static bool onceInit;
};

#endif // APPINFO_H
