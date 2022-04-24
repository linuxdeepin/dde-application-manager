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

#include "desktopinfo.h"
#include "locale.h"
#include "unistd.h"
#include "dstring.h"
#include "dfile.h"
#include "basedir.h"

#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <dirent.h>

std::vector<std::string> DesktopInfo::currentDesktops;

DesktopInfo::DesktopInfo(const std::string &_fileName)
 : kf(KeyFile())
 , fileName(_fileName)
 , isValid(true)
{
    if (!DString::endWith(fileName, ".desktop"))
        fileName += ".desktop";

    if (!DFile::isAbs(fileName)) {
        // fileName是文件名，增加目录
        bool isExisted = false;
        for (const auto &dir : BaseDir::appDirs()) {
            fileName = dir + fileName;
            if (DFile::isExisted(fileName)) {
                isExisted = true;
                break;
            }
        }

        if (!isExisted) {
            isValid = false;
            return;
        }
    }

    kf.loadFile(fileName);

    // check DesktopInfo valid
    if (fileName.find(".desktop") == std::string::npos)
        isValid = false;

    std::vector<std::string> mainKeys = kf.getMainKeys();
    if (mainKeys.size() == 0)
        isValid = false;

    bool found = std::any_of(mainKeys.begin(), mainKeys.end(),
                             [](const auto &key) {return key == MainSection;});

    if (!found)
        isValid = false;

    if (kf.getStr(MainSection, KeyType) != TypeApplication)
        isValid = false;

    name = kf.getLocaleStr(MainSection, KeyName, "");
    icon = kf.getStr(MainSection, KeyIcon);
    id = getId();
}

DesktopInfo::~DesktopInfo()
{

}

std::string DesktopInfo::getFileName()
{
    return fileName;
}

bool DesktopInfo::isValidDesktop()
{
    return isValid;
}

bool DesktopInfo::shouldShow()
{
    if (getNoDisplay() || getIsHidden())
        return false;

    std::vector<std::string> desktopEnvs;
    return getShowIn(desktopEnvs);
}

bool DesktopInfo::getNoDisplay()
{
    return kf.getBool(MainSection, KeyNoDisplay);
}

bool DesktopInfo::getIsHidden()
{
    return kf.getBool(MainSection, KeyHidden);
}

bool DesktopInfo::getShowIn(std::vector<std::string> desktopEnvs)
{
    if (desktopEnvs.size() == 0) {
        if (currentDesktops.size() == 0) {
            const char *env = getenv(envDesktopEnv.c_str());
            const auto &desktop = DString::splitChars(env, ':');
            currentDesktops.assign(desktop.begin(), desktop.end());
        }
        desktopEnvs.assign(currentDesktops.begin(), currentDesktops.end());
    }

    std::vector<std::string> onlyShowIn = kf.getStrList(MainSection, KeyOnlyShowIn);
    std::vector<std::string> notShowIn = kf.getStrList(MainSection, KeyNotShowIn);

    for (const auto &desktop : desktopEnvs) {
        bool ret = std::any_of(onlyShowIn.begin(), onlyShowIn.end(),
                               [&desktop](const auto &d) {return d == desktop;});
        if (ret)
            return true;

        ret = std::any_of(notShowIn.begin(), notShowIn.end(),
                          [&desktop](const auto &d) {return d == desktop;});
        if (ret)
            return false;
    }

    return onlyShowIn.size() == 0;
}

std::string DesktopInfo::getExecutable()
{
    return kf.getStr(MainSection, KeyExec);
}

bool DesktopInfo::isExecutableOk()
{
    // 检查TryExec字段
    std::string value = getTryExec();
    std::vector<std::string> parts = DString::splitStr(value, ' ');
    if (parts.size() > 0 ) {
        value.assign(parts[0]);
        DString::delQuote(value);
        if (strstr(value.c_str(), "/") && DFile::isExisted(value))
            return true;
        else
            return findExecutable(value);
    }

    // 检查Exec字段
    value.assign(getExecutable());
    parts.clear();
    parts = DString::splitStr(value, ' ');
    if (parts.size() > 0) {
        value.assign(parts[0]);
        DString::delQuote(value);
        if (strstr(value.c_str(), "/") && DFile::isExisted(value))
            return true;
        else
            return findExecutable(value);
    }

    return false;
}

bool DesktopInfo::isInstalled()
{
    const char *name = fileName.c_str();
    const char *found = strstr(name, "/applications/");
    if (!found)
        return false;

    auto appDirs = BaseDir::appDirs();
    return std::any_of(appDirs.begin(), appDirs.end(),
                [&name, &found] (std::string dir) -> bool {return strneq(dir.c_str(), name, size_t(found - name));});
}

std::vector<DesktopAction> DesktopInfo::getActions()
{
    std::vector<DesktopAction> actions;
    for (const auto &mainKey : kf.getMainKeys()) {
        if (DString::startWith(mainKey, "Desktop Action")
                || DString::endWith(mainKey, "Shortcut Group")) {
            DesktopAction action;
            action.name = kf.getLocaleStr(mainKey, KeyName, "");
            action.exec = kf.getStr(mainKey, KeyExec);
            actions.push_back(action);
        }
    }

    return actions;
}

// 使用appId获取DesktopInfo需检查有效性
DesktopInfo DesktopInfo::getDesktopInfoById(std::string appId)
{
    if (!DString::endWith(appId, ".desktop"))
        appId += ".desktop";

    for (const auto & dir : BaseDir::appDirs()) {
        std::string filePath = dir + appId;
        //检测文件有效性
        if (DFile::isExisted(filePath)) {
            return DesktopInfo(filePath);
        }
    }

    return DesktopInfo("");
}

// TryExec is Path to an executable file on disk used to determine if the program is actually installed
std::string DesktopInfo::getTryExec()
{
    return kf.getStr(MainSection, KeyTryExec);
}

// 按$PATH路径查找执行文件
bool DesktopInfo::findExecutable(std::string &exec)
{
    static const char *path = getenv("PATH");
    static std::vector<std::string> paths = DString::splitChars(path, ':');
    return std::any_of(paths.begin(), paths.end(), [&exec](std::string path) {return DFile::isExisted(path + "/" +exec);});
}

// filename must has suffix desktopExt
// example:
// /usr/share/applications/a.desktop -> a
// /usr/share/applications/kde4/a.desktop -> kde4/a
// /xxxx/dir/a.desktop -> /xxxx/dir/a
std::string DesktopInfo::getId()
{
    if (!id.empty())
        return id;

    std::string idStr;
    auto const suffixPos = fileName.find(".desktop");
    if (suffixPos == std::string::npos)
        return "";

    idStr = fileName.substr(0, fileName.size() - 8); // trim suffix
    size_t dirPos = idStr.find("/applications/");
    if (dirPos == std::string::npos)
        return "";

    std::string baseDir(idStr.substr(0, dirPos + 14)); // length of "/applications/" is 14
    std::vector<std::string> appDirs = BaseDir::appDirs();
    bool installed = std::any_of(appDirs.begin(), appDirs.end(),
                                 [&baseDir](const auto &dir) {return dir == baseDir;});

    if (installed) {
        id = idStr.substr(baseDir.size(), idStr.size());
    }

    return id;
}

std::string DesktopInfo::getGenericName()
{
    return kf.getLocaleStr(MainSection, KeyGenericName, "");
}

std::string DesktopInfo::getName()
{
    return name;
}

std::string DesktopInfo::getIcon()
{
    return icon;
}

std::string DesktopInfo::getCommandLine()
{
    return kf.getStr(MainSection, KeyExec);
}

std::vector<std::string> DesktopInfo::getKeywords()
{
    return kf.getLocaleStrList(MainSection, KeyKeywords, "");
}

std::vector<std::string> DesktopInfo::getCategories()
{
    return kf.getStrList(MainSection, KeyCategories);
}

// class AppsDir
AppsDir::AppsDir(const std::string &dirPath)
 : path(dirPath)
{

}

AppsDir::~AppsDir()
{

}

std::string AppsDir::getPath()
{
    return path;
}


// 获取目录对应的应用名称
std::map<std::string, bool> AppsDir::getAppNames()
{
    DIR* dp;
    struct dirent* ep;

    dp = opendir(path.c_str());
    if (dp == nullptr)
    {
        std::cout << "Couldn't open directory " << path << std::endl;
        return appNames;
    }

    while ((ep = readdir(dp))) {
        if (ep->d_type != DT_REG && ep->d_type != DT_LNK)
            continue;

        if (!DString::endWith(ep->d_name, ".desktop"))
            continue;

        appNames.insert({ep->d_name, true});
    }
    closedir(dp);

    return appNames;
}

// 获取所有应用信息
std::vector<DesktopInfo> AppsDir::getAllDesktopInfos()
{
    std::map<std::string, bool> recoder;
    std::vector<DesktopInfo> desktopInfos;

    for (auto dir : BaseDir::appDirs()) {
        AppsDir appsDir(dir);
        std::map<std::string, bool> appNames = appsDir.getAppNames();
        if (appNames.size() == 0)
            continue;

        for (const auto &iter : appNames) {
            if (recoder.find(iter.first) != recoder.end())
                continue;

            std::string filePath =  dir + iter.first;
            DesktopInfo desktopInfo(filePath);
            if (!desktopInfo.isValidDesktop())
                continue;

            if (!desktopInfo.shouldShow())
                continue;

            desktopInfos.push_back(std::move(desktopInfo));
            recoder[iter.first] = true;
        }
    }

    return desktopInfos;
}

