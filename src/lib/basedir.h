// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEDIR_H
#define BASEDIR_H

#include <vector>
#include <string>

// 基础目录类， 目录结尾统一包含斜杠/
class BaseDir
{
public:
    BaseDir();

    static std::string homeDir();
    static std::string userDataDir();
    static std::vector<std::string> sysDataDirs();
    static std::string userConfigDir();
    static std::vector<std::string> sysConfigDirs();
    static std::string userCacheDir();
    static std::string userAppDir();
    static std::vector<std::string> sysAppDirs();
    static std::vector<std::string> appDirs();
    static std::vector<std::string> autoStartDirs();
    static std::string userAutoStartDir();

private:
    static void filterNotAbs(std::vector<std::string>& dirs);
    static void addSuffixSlash(std::vector<std::string>& dirs);
};

#endif // BASEDIR_H
