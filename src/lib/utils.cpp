// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"

#define HOME "HOME"

#include <unistd.h>
#include <fcntl.h>

std::string getUserDir(const char* envName);
std::vector<std::string> getSystemDirs(const char* envName);

std::string getUserHomeDir()
{
    const char* dir = getenv(HOME);

    if (dir) {
        return dir;
    }

    struct passwd* user = getpwent();
    if (user) {
        return user->pw_dir;
    }

    return "";
}

std::string getUserDataDir()
{
    // default $HOME/.local/share
    std::string userDataDir = getUserDir("XDG_DATA_HOME");

    if (userDataDir.empty()) {
        userDataDir = getUserHomeDir();
        if (!userDataDir.empty()) {
            userDataDir += "/.local/share";
        }
    }
    return userDataDir;
}

std::string getUserConfigDir()
{
    // default $HOME/.config
    std::string userConfigDir = getUserDir("XDG_CONFIG_HOME");

    if (userConfigDir.empty()) {
        userConfigDir = getUserHomeDir();
        if (!userConfigDir.empty()) {
            userConfigDir += "/.config";
        }

    }
    return userConfigDir;
}

std::string getUserDir(const char* envName)
{
    const char* envDir = getenv(envName);
    if (!envDir) {
        return "";
    }

    if (!QDir::isAbsolutePath(envDir)) {
        return "";
    }

    return envDir;
}

std::vector<std::string> getSystemDataDirs()
{
    std::vector<std::string> systemDir = getSystemDirs("XDG_DATA_DIRS");

    if (systemDir.empty()) {
        systemDir.push_back({"/usr/local/share", "/usr/share"});
    }

    return systemDir;
}

std::vector<std::string> getSystemConfigDirs()
{
    std::vector<std::string> systemDir = getSystemDirs("XDG_CONFIG_DIRS");

    if (systemDir.empty()) {
        systemDir.push_back("/etc/xdg");
    }

    return systemDir;
}

std::vector<std::string> getSystemDirs(const char* envName)
{
    std::vector<std::string> dirVector;
    const char* envDir = getenv(envName);
    if (envDir == nullptr) {
        return dirVector;
    }

    QString tempDirs(envDir);
    auto tempList = tempDirs.split(":");

    for (auto iter : tempList) {
        if (QDir::isAbsolutePath(iter)) {
            dirVector.push_back(iter.toStdString());
        }
    }


    return dirVector;
}

std::string lookPath(std::string file)
{
    std::string path;

    if (file.find("/") != std::string::npos) {
        if (access(path.c_str(), X_OK) != -1) {
            return file;
        } else {
            return path;
        }
    }

    char* pathEnv = getenv("PATH");

    char* temp = strtok(pathEnv, ";");

    while (temp) {
        path = std::string(temp) + "/" + file;

        if (access(path.c_str(), X_OK) != -1) {
            return path;
        } else {
            path = "";
            temp = strtok(nullptr, "/");
        }
    }

    return path;
}

void walk(std::string root, std::vector<std::string>& skipdir, std::map<std::string, int>& retMap)
{
    walk(root, ".", skipdir, retMap);
}

void walk(std::string root, std::string name, std::vector<std::string>& skipdir, std::map<std::string, int>& retMap)
{
    QDir dir(root.c_str());

    if (dir.exists()) {
        if (std::find(skipdir.begin(), skipdir.end(), name) != skipdir.end()) {
            return;
        }
    } else {
        return;
    }

    if (hasEnding(name, ".desktop")) {
        retMap[name] = 0;
    }

    std::string path = root + "/" + name;
    QDir temp(path.c_str());

    QStringList entryList = temp.entryList();
    for (auto iter : entryList) {
        QFile file(iter);

        if (file.exists()) {
            continue;
        }
        walk(root, name + "/" + iter.toStdString(), skipdir, retMap);
    }

}

bool hasEnding(std::string const& fullString, std::string const& ending)
{
    if (fullString.length() >= ending.length()) {
        return fullString.compare(fullString.length() - ending.length(), ending.length(), ending) == 0;
    } else {
        return false;
    }
}

bool hasBeginWith(std::string const& fullString, std::string const& ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(0, ending.length(), ending));
    } else {
        return false;
    }
}
