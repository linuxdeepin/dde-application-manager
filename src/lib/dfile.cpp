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

#include "dfile.h"
#include "macro.h"

#include <unistd.h>
#include <cstring>

DFile::DFile()
{

}

bool DFile::isAbs(std::string file)
{
    char resolved_path[MAX_FILEPATH_LEN];
    if (realpath(file.c_str(), resolved_path)) {
        std::string filePath(resolved_path);
        if (filePath == file)
            return true;
    }

    return false;
}

bool DFile::isExisted(std::string file)
{
    return !access(file.c_str(), F_OK);
}

std::string DFile::dir(std::string file)
{
    std::string ret;
    if (isAbs(file)) {
        size_t pos = file.find_last_of("/");
        if (pos != std::string::npos) {
            ret.assign(file, 0, pos + 1); // 包含结尾斜杠/
        }
    }

    return ret;
}

std::string DFile::base(std::string file)
{
    std::string ret;
    if (strstr(file.c_str(), "/")) {    // 包含路径
        size_t pos = file.find_last_of("/");
        if (pos != std::string::npos) {
            ret.assign(file, pos + 1, file.size() - pos);   // 去除路径
        }
    }

    size_t pos = file.find_last_of(".");    // 去除后缀
    if (pos != std::string::npos) {
        ret.assign(file, 0, pos + 1);
    }

    return ret;
}
