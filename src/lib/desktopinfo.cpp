// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktopinfo.h"
#include "locale.h"
#include "unistd.h"
#include "dstring.h"
#include "dfile.h"
#include "basedir.h"

#include <QDebug>

#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <dirent.h>

std::vector<std::string> DesktopInfo::currentDesktops;

DesktopInfo::DesktopInfo(const std::string &_fileName)
    : m_isValid(true)
    , m_keyFile(KeyFile())
{
    std::string fileNameWithSuffix(_fileName);
    if (!DString::endWith(_fileName, ".desktop"))
        fileNameWithSuffix += ".desktop";

    m_fileName = fileNameWithSuffix;

    if (DFile::dir(m_fileName).empty()) {
        // fileName是文件名，增加目录
        bool isExisted = false;
        for (const auto &dir : BaseDir::appDirs()) {
            m_fileName = dir + fileNameWithSuffix;
            if (DFile::isExisted(m_fileName)) {
                isExisted = true;
                break;
            }
        }

        if (!isExisted) {
            m_isValid = false;
            return;
        }
    }

    m_keyFile.loadFile(m_fileName);

    // check DesktopInfo valid
    std::vector<std::string> mainKeys = m_keyFile.getMainKeys();
    if (mainKeys.size() == 0)
        m_isValid = false;

    bool found = std::any_of(mainKeys.begin(), mainKeys.end(),
                             [](const auto &key) {return key == MainSection;});

    if (!found)
        m_isValid = false;

    if (m_keyFile.getStr(MainSection, KeyType) != TypeApplication)
        m_isValid = false;

    m_name = m_keyFile.getLocaleStr(MainSection, KeyName, "");
    m_icon = m_keyFile.getStr(MainSection, KeyIcon);
    m_id = getId();
}

DesktopInfo::~DesktopInfo()
{

}

std::string DesktopInfo::getFileName()
{
    return m_fileName;
}

bool DesktopInfo::isValidDesktop()
{
    return m_isValid;
}

/** if return true, item is shown
 * @brief DesktopInfo::shouldShow
 * @return
 */
bool DesktopInfo::shouldShow()
{
    if (getNoDisplay() || getIsHidden()) {
        qDebug() << "hidden desktop file path: " << QString::fromStdString(m_fileName);
        return false;
    }

    std::vector<std::string> desktopEnvs;
    return getShowIn(desktopEnvs);
}

bool DesktopInfo::getNoDisplay()
{
    return m_keyFile.getBool(MainSection, KeyNoDisplay);
}

bool DesktopInfo::getIsHidden()
{
    return m_keyFile.getBool(MainSection, KeyHidden);
}

bool DesktopInfo::getShowIn(std::vector<std::string> desktopEnvs)
{
#ifdef QT_DEBUG
    qDebug() << "desktop file path: " << QString::fromStdString(m_fileName);
#endif

    if (desktopEnvs.size() == 0) {
        const char *env = getenv(envDesktopEnv.c_str());
        const auto &desktop = DString::splitChars(env, ':');
        currentDesktops.assign(desktop.begin(), desktop.end());
        desktopEnvs.assign(currentDesktops.begin(), currentDesktops.end());
    }

    std::vector<std::string> onlyShowIn = m_keyFile.getStrList(MainSection, KeyOnlyShowIn);
    std::vector<std::string> notShowIn = m_keyFile.getStrList(MainSection, KeyNotShowIn);

#ifdef QT_DEBUG
    auto strVector2qstrVector = [](const std::vector<std::string> &vector) {
        QVector<QString> vectorString;
        for (const std::string &str : vector)
            vectorString.append(QString::fromStdString(str));

        return vectorString;
    };

    qDebug() << "onlyShowIn:" << strVector2qstrVector(onlyShowIn) <<
                ", notShowIn:" << strVector2qstrVector(notShowIn) <<
                ", desktopEnvs:" << strVector2qstrVector(desktopEnvs);
#endif

    for (const auto &desktop : desktopEnvs) {
        bool ret = std::any_of(onlyShowIn.begin(), onlyShowIn.end(),
                               [&desktop](const auto &d) {return d == desktop;});
#ifdef QT_DEBUG
        qInfo() << Q_FUNC_INFO << "onlyShowIn, result:" << ret;
#endif
        if (ret)
            return true;

        ret = std::any_of(notShowIn.begin(), notShowIn.end(),
                          [&desktop](const auto &d) {return d == desktop;});
#ifdef QT_DEBUG
        qInfo() << Q_FUNC_INFO << "notShowIn, result:" << ret;
#endif
        if (ret)
            return false;
    }

    return onlyShowIn.size() == 0;
}

std::string DesktopInfo::getExecutable()
{
    return m_keyFile.getStr(MainSection, KeyExec);
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
    const char *name = m_fileName.c_str();
    const char *found = strstr(name, "/applications/");
    if (!found)
        return false;

    auto appDirs = BaseDir::appDirs();
    return std::any_of(appDirs.begin(), appDirs.end(),
                       [&name, &found] (std::string dir) -> bool {return strneq(dir.c_str(), name, size_t(found - name));});
}

// [Desktop Action new-window] or [Full_Screenshot Shortcut Group]
bool DesktopInfo::isDesktopAction(std::string name)
{
    return DString::startWith(name.c_str(), "Desktop Action") || DString::endWith(name.c_str(), "Shortcut Group");
}

std::vector<DesktopAction> DesktopInfo::getActions()
{
    std::vector<DesktopAction> actions;
    for (const auto &mainKey : m_keyFile.getMainKeys()) {
        if (DString::startWith(mainKey, "Desktop Action")
                || DString::endWith(mainKey, "Shortcut Group")) {
            DesktopAction action;
            action.name = m_keyFile.getLocaleStr(mainKey, KeyName, "");
            action.exec = m_keyFile.getStr(mainKey, KeyExec);
            action.section = mainKey;
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

bool DesktopInfo::getTerminal()
{
    return m_keyFile.getBool(MainSection, KeyTerminal);
}

// TryExec is Path to an executable file on disk used to determine if the program is actually installed
std::string DesktopInfo::getTryExec()
{
    return m_keyFile.getStr(MainSection, KeyTryExec);
}

// 按$PATH路径查找执行文件
bool DesktopInfo::findExecutable(std::string &exec)
{
    static const char *path = getenv("PATH");
    static std::vector<std::string> paths = DString::splitChars(path, ':');
    return std::any_of(paths.begin(), paths.end(), [&exec](std::string path) {return DFile::isExisted(path + "/" +exec);});
}

/**
 * @brief DesktopInfo::getId
 * filename must has suffix desktopExt
 * example:
 * /usr/share/applications/a.desktop -> a
 * /usr/share/applications/kde4/a.desktop -> kde4/a
 * /xxxx/dir/a.desktop -> /xxxx/dir/a
 * @return
 */
std::string DesktopInfo::getId()
{
    if (!m_id.empty())
        return m_id;

    std::string idStr;
    auto const suffixPos = m_fileName.find(".desktop");
    if (suffixPos == std::string::npos)
        return "";

    idStr = m_fileName.substr(0, m_fileName.size() - 8); // trim suffix
    size_t dirPos = idStr.find("/applications/");
    if (dirPos == std::string::npos)
        return idStr;

    std::string baseDir(idStr.substr(0, dirPos + 14)); // length of "/applications/" is 14
    std::vector<std::string> appDirs = BaseDir::appDirs();
    bool installed = std::any_of(appDirs.begin(), appDirs.end(),
                                 [&baseDir](const auto &dir) {return dir == baseDir;});

    if (installed) {
        m_id = idStr.substr(baseDir.size(), idStr.size());
    }

    return m_id;
}

std::string DesktopInfo::getGenericName()
{
    return m_keyFile.getLocaleStr(MainSection, KeyGenericName, "");
}

std::string DesktopInfo::getName()
{
    return m_name;
}

std::string DesktopInfo::getIcon()
{
    return m_icon;
}

std::string DesktopInfo::getCommandLine()
{
    return m_keyFile.getStr(MainSection, KeyExec);
}

std::vector<std::string> DesktopInfo::getKeywords()
{
    return m_keyFile.getLocaleStrList(MainSection, KeyKeywords, "");
}

std::vector<std::string> DesktopInfo::getCategories()
{
    return m_keyFile.getStrList(MainSection, KeyCategories);
}

void DesktopInfo::setDesktopOverrideExec(const std::string &execStr)
{
    m_overRideExec = execStr;
}

KeyFile *DesktopInfo::getKeyFile()
{
    return &m_keyFile;
}

// class AppsDir
AppsDir::AppsDir(const std::string &dirPath)
    : m_path(dirPath)
{

}

AppsDir::~AppsDir()
{

}

std::string AppsDir::getPath()
{
    return m_path;
}


// 获取目录对应的应用名称
std::map<std::string, bool> AppsDir::getAppNames()
{
    DIR* dp;
    struct dirent* ep;

    dp = opendir(m_path.c_str());
    if (!dp) {
        std::cout << "Couldn't open directory " << m_path << std::endl;
        return m_appNames;
    }

    while ((ep = readdir(dp))) {
        if (ep->d_type != DT_REG && ep->d_type != DT_LNK)
            continue;

        if (!DString::endWith(ep->d_name, ".desktop"))
            continue;

        m_appNames.insert({ep->d_name, true});
    }
    closedir(dp);

    return m_appNames;
}

// 获取所有应用信息
std::vector<DesktopInfo> AppsDir::getAllDesktopInfos()
{
    std::vector<DesktopInfo> desktopInfos;

    for (auto dir : BaseDir::appDirs()) {
        AppsDir appsDir(dir);
        std::map<std::string, bool> appNames = appsDir.getAppNames();
        if (appNames.size() == 0)
            continue;

        for (const auto &iter : appNames) {
            std::string filePath =  dir + iter.first;

            DesktopInfo desktopInfo(filePath);
            if (!DFile::isExisted(filePath) || !desktopInfo.isValidDesktop() || !desktopInfo.shouldShow()) {
                qDebug() << QString("app item %1 doesn't show in the list..").arg(QString::fromStdString(filePath));
                continue;
            }

            desktopInfos.push_back(desktopInfo);
        }
    }

    return desktopInfos;
}
