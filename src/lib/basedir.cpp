// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "basedir.h"
#include "dfile.h"
#include "dstring.h"

#include <algorithm>

BaseDir::BaseDir()
{

}

std::string BaseDir::homeDir()
{
    char *home = getenv("HOME");
    if (!home)
        return "";

    return std::string(home) + "/";
}

std::string BaseDir::userDataDir()
{
    // default $HOME/.local/share
    std::string home = homeDir();
    std::string defaultDir = (home.size() > 0) ? home + ".local/share/" : "";
    const char *xdgDataHomePtr = getenv("XDG_DATA_HOME");
    if (!xdgDataHomePtr)
        return defaultDir;

    if (!DFile::isDir(xdgDataHomePtr))
        return defaultDir;

    return std::string(xdgDataHomePtr) + "/";
}

std::vector<std::string> BaseDir::sysDataDirs()
{
    std::vector<std::string> defaultDirs {"/usr/local/share/", "/usr/share/"};
    const char *xdgDataDirsPtr = getenv("XDG_DATA_DIRS");
    if (!xdgDataDirsPtr)
        return defaultDirs;

    std::string xdgDataDirsStr(xdgDataDirsPtr);
    std::vector<std::string> xdgDataDirs = DString::splitStr(xdgDataDirsStr, ':');
    if (xdgDataDirs.size() == 0)
        return defaultDirs;

    filterNotAbs(xdgDataDirs);
    addSuffixSlash(xdgDataDirs);
    return  xdgDataDirs;
}

std::string BaseDir::userConfigDir()
{
    // default $HOME/.config
    std::string defaultDir = homeDir() + ".config/";
    const char *xdgConfigHomePtr = getenv("XDG_CONFIG_HOME");
    if (!xdgConfigHomePtr)
        return defaultDir;

    std::string xdgConfigHome(xdgConfigHomePtr);
    if (!DFile::isDir(xdgConfigHome))
        return defaultDir;

    return xdgConfigHome + "/";
}

std::vector<std::string> BaseDir::sysConfigDirs()
{
    std::vector<std::string> defaultDirs {"/etc/xdg/"};
    const char *xdgConfigDirsPtr = getenv("XDG_CONFIG_DIRS");
    if (!xdgConfigDirsPtr)
        return defaultDirs;

    std::string xdgConfigDirsStr(xdgConfigDirsPtr);
    std::vector<std::string> xdgConfigDirs = DString::splitStr(xdgConfigDirsStr, ':');
    if (xdgConfigDirs.size() == 0)
        return defaultDirs;

    filterNotAbs(xdgConfigDirs);
    addSuffixSlash(xdgConfigDirs);
    return xdgConfigDirs;
}

std::string BaseDir::userCacheDir()
{
    std::string home = homeDir();
    std::string defaultDir = (home.size() > 0) ? home + ".cache/" : "";
    const char *xdgCacheHomePtr = getenv("XDG_CACHE_HOME");
    if (!xdgCacheHomePtr)
        return  defaultDir;

    std::string xdgCacheHome(xdgCacheHomePtr);
    if (!DFile::isDir(xdgCacheHome))
        return defaultDir;

    return xdgCacheHome + "/";
}

std::string BaseDir::userAppDir()
{
    std::string dataDir = userDataDir();
    return (dataDir.size() > 0) ?  dataDir + "applications/" : "";
}

std::vector<std::string> BaseDir::sysAppDirs()
{
    auto dataDirs = sysDataDirs();
    std::vector<std::string> sysAppDirs(dataDirs.size());
    std::transform(dataDirs.begin(), dataDirs.end(), sysAppDirs.begin(),
                   [](std::string dir) -> std::string {return dir + "applications/";});
    return sysAppDirs;
}

std::vector<std::string> BaseDir::appDirs()
{
    std::vector<std::string> appDirs = sysAppDirs();
    appDirs.push_back(userAppDir());
    return appDirs;
}

std::vector<std::string> BaseDir::autoStartDirs()
{
    std::vector<std::string> autoStartDirs = sysConfigDirs();
    autoStartDirs.push_back(userConfigDir());
    std::transform(autoStartDirs.begin(), autoStartDirs.end(), autoStartDirs.begin(),
                   [](std::string dir) -> std::string {return dir + "autostart/";});

    return autoStartDirs;
}

std::string BaseDir::userAutoStartDir()
{
    return userConfigDir() + "autostart/";
}

void BaseDir::filterNotAbs(std::vector<std::string> &dirs)
{
    for (auto iter = dirs.begin(); iter != dirs.end();) {       // erase element in vector
        if (!DFile::isDir(*iter))
            iter = dirs.erase(iter);
        else
            iter++;
    }
}

void BaseDir::addSuffixSlash(std::vector<std::string> &dirs)
{
    for (auto &dir : dirs) {
        if (!DString::endWith(dir, "/"))
            dir += "/";
    }
}
