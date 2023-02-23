// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appinfo.h"
#include "utils.h"
#include "dlocale.h"
#include "appinfocommon.h"

#include <string>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <string.h>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

bool AppInfoManger::onceInit = false;
std::vector<std::string> AppInfoManger::xdgDataDirs;
std::vector<std::string> AppInfoManger::xdgAppDirs;

AppInfoManger::AppInfoManger()
{
    canDelete = false;
}

std::vector<std::string> AppInfoManger::getStringList(std::string session, std::string key)
{
    return keyFile.getStrList(session, key);
}

std::string AppInfoManger::toJson()
{
    QJsonDocument doc;
    QJsonObject obj;

    obj["Id"] = desktopId.c_str();
    obj["Name"] = appName.c_str();
    obj["DisplayName"] = displayName.c_str();
    obj["Description"] = comment.c_str();
    obj["Icon"] = icon.c_str();
    obj["Exec"] = cmdline.c_str();
    obj["CanDelete"] = canDelete;

    if (!obj.isEmpty()) {
        doc.setObject(obj);
    }

    return doc.toJson(QJsonDocument::Compact).toStdString();
}

std::string AppInfoManger::getFileName()
{
    return fileName;
}

std::shared_ptr<AppInfoManger> AppInfoManger::loadByDesktopId(std::string desktopId)
{
    std::shared_ptr<AppInfoManger> appInfo;

    if (!hasEnding(desktopId, AppinfoCommon::DesktopExt)) {
        desktopId += AppinfoCommon::DesktopExt;
    }

    if (QDir::isAbsolutePath(desktopId.c_str())) {
        appInfo = AppInfoManger::initAppInfoByFile(desktopId);
        if (appInfo) {
            appInfo->desktopId = AppInfoManger::getDesktopIdFile(desktopId);
        }
        return appInfo;
    }

    std::vector<std::string> appDirs = AppInfoManger::getXdgAppsDirs();

    for (auto iter : appDirs) {
        appInfo = AppInfoManger::initAppInfoByFile(iter + "/" + desktopId);
        if (appInfo) {
            appInfo->desktopId = desktopId;
            return appInfo;
        }
    }

    return appInfo;
}

std::shared_ptr<AppInfoManger> AppInfoManger::initAppInfoByFile(std::string fileName)
{
    std::shared_ptr<AppInfoManger> appInfo;
    KeyFile keyFile;

    bool bSuccess = keyFile.loadFile(fileName);
    if (!bSuccess) {
        return appInfo;
    }

    std::string type = keyFile.getStr(AppinfoCommon::MainSection, AppinfoCommon::KeyType);

    if (type != AppinfoCommon::TypeApplication) {
        return appInfo;
    }

    appInfo = std::make_shared<AppInfoManger>();

    appInfo->appName = getAppName(keyFile);
    appInfo->icon = AppInfoManger::getIconByKeyFile(keyFile);
    appInfo->displayName = appInfo->appName;
    appInfo->comment = keyFile.getLocaleStr(AppinfoCommon::MainSection, AppinfoCommon::KeyComment);
    appInfo->cmdline = keyFile.getStr(AppinfoCommon::MainSection, AppinfoCommon::KeyExec);
    appInfo->fileName = fileName;
    appInfo->keyFile = keyFile;

    if (!appInfo->shouldShow()) {
        return nullptr;
    }

    return appInfo;
}

std::string AppInfoManger::getIconByKeyFile(KeyFile& keyFile)
{
    std::string icon = keyFile.getStr(AppinfoCommon::MainSection, AppinfoCommon::KeyIcon);

    if (!QDir::isAbsolutePath(icon.c_str())) {
        if (hasEnding(icon, ".png")) {
            icon = icon.substr(0, icon.length() - strlen(".png"));
        }

        if (hasEnding(icon, ".xpm")) {
            icon = icon.substr(0, icon.length() - strlen(".xpm"));
        }

        if (hasEnding(icon, ".svg")) {
            icon = icon.substr(0, icon.length() - strlen(".svg"));
        }
    }

    return icon;
}

std::string AppInfoManger::getDesktopIdFile(std::string fileName)
{
    if (hasEnding(fileName, AppinfoCommon::DesktopExt)) {
        fileName = fileName.substr(0, fileName.size() - strlen(AppinfoCommon::DesktopExt.c_str()) + 1);
    }

    auto index = fileName.find("/applications/");
    if (index == fileName.npos) {
        return "";
    }

    std::string dir = fileName.substr(0, index);

    std::vector<std::string> dataDIrs = AppInfoManger::getXdgDataDirs();

    if (std::find(dataDIrs.begin(), dataDIrs.end(), dir) != dataDIrs.end()) {

        return fileName.substr(index + 14, fileName.size() - (index + 14 + 1));
    }

    return fileName;
}

bool AppInfoManger::shouldShow()
{
    if (!keyFile.getStr(AppinfoCommon::MainSection, AppinfoCommon::KeyNoDisplay).empty()) {
        return false;
    }

    if (!keyFile.getStr(AppinfoCommon::MainSection, AppinfoCommon::KeyHidden).empty()) {
        return false;
    }
    QString  deskEnv = getenv("XDG_CURRENT_DESKTOP");

    auto envList = deskEnv.split(":");

    std::vector<std::string> onlyShowIn = keyFile.getStrList(AppinfoCommon::MainSection, AppinfoCommon::KeyOnlyShowIn);
    std::vector<std::string> notShowIn = keyFile.getStrList(AppinfoCommon::MainSection, AppinfoCommon::KeyNotShowIn);

    for (auto iter : envList) {
        if (std::find(onlyShowIn.begin(), onlyShowIn.end(), iter.toStdString()) != onlyShowIn.end()) {
            return true;
        }
        if (std::find(notShowIn.begin(), notShowIn.end(), iter.toStdString()) != notShowIn.end()) {
            return false;
        }
    }

    return onlyShowIn.empty();
}

std::string AppInfoManger::getDefaultApp(std::string mimeType, bool supportUri)
{
    GAppInfo* gAppInfo = g_app_info_get_default_for_type(mimeType.c_str(), supportUri);

    if (gAppInfo == nullptr) {
        return "";
    }

    if (supportUri && !g_app_info_supports_uris(gAppInfo)) {
        return "";
    }

    return g_app_info_get_id(gAppInfo);
}

std::vector<std::string> AppInfoManger::getAppList(std::string mimeType)
{
    std::vector<std::string> retVector;
    GList* appInfoList = g_app_info_get_all_for_type(mimeType.c_str());

    while (appInfoList) {
        GAppInfo* gAppInfo = static_cast<GAppInfo*>(appInfoList->data);

        const char* appId = g_app_info_get_id(gAppInfo);
        if (appId) {
            retVector.push_back(appId);
        }
        appInfoList = appInfoList->next;
    }

    return  retVector;
}


bool AppInfoManger::setDefaultApp(std::string mimeType, std::string desktopId)
{
    GDesktopAppInfo* gDesktopAppInfo = g_desktop_app_info_new(desktopId.c_str());

    if (gDesktopAppInfo == nullptr) {
        return false;
    }

    GAppInfo* gAppInfo = G_APP_INFO(gDesktopAppInfo);

    GError* err = nullptr;
    g_app_info_set_as_default_for_type(gAppInfo, mimeType.c_str(), &err);

    if (err != nullptr) {
        return false;
    }
    return true;
}

std::string AppInfoManger::getAppName(KeyFile& keyFile)
{
    std::string name;

    std::string xDeepinVendor = keyFile.getStr(AppinfoCommon::MainSection, AppinfoCommon::DeepinVendor);

    if (xDeepinVendor == "deepin") {
        name = keyFile.getLocaleStr(AppinfoCommon::MainSection, AppinfoCommon::KeyGenericName);
    } else {
        name = keyFile.getLocaleStr(AppinfoCommon::MainSection, AppinfoCommon::KeyName);
    }

    if (name.empty()) {
        name = AppInfoManger::getDesktopIdFile(keyFile.getFilePath());
    }

    return name;
}

std::vector<std::string>& AppInfoManger::getXdgDataDirs()
{
    if (!AppInfoManger::onceInit) {
        AppInfoManger::onceInit = true;

        AppInfoManger::xdgDataDirs.push_back(getUserDataDir());
        for (auto iter : getSystemDataDirs()) {
            xdgDataDirs.push_back(iter);
        }

        for (auto dataDir : AppInfoManger::xdgDataDirs) {
            AppInfoManger::xdgAppDirs.push_back(dataDir + "/applications");
        }
    }

    return AppInfoManger::xdgDataDirs;
}

std::vector<std::string>& AppInfoManger::getXdgAppsDirs()
{
    if (!AppInfoManger::onceInit) {
        AppInfoManger::onceInit = true;

        AppInfoManger::xdgDataDirs.push_back(getUserDataDir());

        for (auto iter : getSystemDataDirs()) {
            xdgDataDirs.push_back(iter);
        }

        for (auto dataDir : AppInfoManger::xdgDataDirs) {
            AppInfoManger::xdgAppDirs.push_back(dataDir + "/applications");
        }
    }

    return AppInfoManger::xdgAppDirs;
}

std::vector<std::shared_ptr<AppInfoManger>> AppInfoManger::getAll(std::map<std::string, std::vector<std::string>> skipDirs)
{
    std::vector<std::shared_ptr<AppInfoManger>> ret;

    std::vector<std::string> xdgAppDirs = AppInfoManger::getXdgAppsDirs();

    std::vector<std::pair<std::string, std::map<std::string, int>>> appPathNameInfos;
    for (auto iter : xdgAppDirs) {
        std::map<std::string, int> tempMap;
        if (skipDirs.count(iter) != 0) {
            walk(iter, skipDirs[iter], tempMap);
        } else {
            std::vector<std::string> temp;
            walk(iter, temp, tempMap);
        }
        appPathNameInfos.push_back(std::make_pair(iter, tempMap));
    }

    std::map<std::string, int> recordMap;

    for (auto appPathNameInfo : appPathNameInfos) {
        for (auto appName : appPathNameInfo.second) {
            std::string tempAppName = appName.first;
            if (hasBeginWith(appName.first, "./")) {
                tempAppName = appName.first.substr(appName.first.find_first_of("./") + 2, appName.first.size() - 2);
            }
            if (recordMap.count(tempAppName)) {
                continue;
            }

            std::shared_ptr<AppInfoManger> appInfo = AppInfoManger::loadByDesktopId(appPathNameInfo.first + "/" + tempAppName);
            if (!appInfo) {
                continue;
            }

            recordMap[tempAppName] = 0;

            if (appInfo->shouldShow()) {
                ret.push_back(appInfo);
            }
        }
    }
    return ret;
}


std::string AppInfoManger::getDesktopId()
{
    return desktopId;
}

std::string AppInfoManger::getAppName()
{
    return appName;
}
std::string AppInfoManger::getDisplayName()
{
    return displayName;
}
std::string AppInfoManger::getComment()
{
    return comment;
}
std::string AppInfoManger::getIcon()
{
    return icon;
}

std::string AppInfoManger::getCmdline()
{
    return cmdline;
}

bool AppInfoManger::getCanDelete()
{
    return canDelete;
}

void AppInfoManger::setCanDelete(bool bCanDelete)
{
    canDelete = bCanDelete;
}
std::vector<std::string> AppInfoManger::getCategories()
{
    return keyFile.getStrList(AppinfoCommon::MainSection, AppinfoCommon::KeyCategories);
}

void AppInfoManger::setDesktopId(std::string desktopId)
{
    this->desktopId = desktopId;
}

std::string AppInfoManger::toJson(std::vector<std::shared_ptr<AppInfoManger>> appInfos)
{
    QJsonDocument doc;
    QJsonObject obj;
    QJsonArray arr;
    for (auto iter : appInfos) {
        obj["Id"] = iter->getDesktopId().c_str();
        obj["Name"] = iter->getAppName().c_str();
        obj["DisplayName"] = iter->getDisplayName().c_str();
        obj["Description"] = iter->getComment().c_str();
        obj["Icon"] = iter->getIcon().c_str();
        obj["Exec"] = iter->getCmdline().c_str();
        obj["CanDelete"] = iter->getCanDelete();
        arr.push_back(obj);
    }

    if (!arr.empty()) {
        doc.setArray(arr);
    }

    return doc.toJson(QJsonDocument::Compact).toStdString();
}
