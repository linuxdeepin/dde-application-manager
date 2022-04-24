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
    static std::string uerDataDir();
    static std::vector<std::string> sysDataDirs();
    static std::string userConfigDir();
    static std::vector<std::string> sysConfigDirs();
    static std::string userCacheDir();
    static std::string userAppDir();
    static std::vector<std::string> sysAppDirs();
    static std::vector<std::string> appDirs();
    static std::vector<std::string> autoStartDirs();

private:
    static void filterNotAbs(std::vector<std::string> &dirs);
    static void addSuffixSlash(std::vector<std::string> &dirs);
};

#endif // BASEDIR_H
