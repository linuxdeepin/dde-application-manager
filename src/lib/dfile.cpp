// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dfile.h"
#include "macro.h"
#include "dstring.h"

#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

DFile::DFile()
{

}

// 符号连接
bool DFile::isLink(std::string file)
{
    if (file.empty())
        return false;

    struct stat fileStat;
    if (!stat(file.c_str(), &fileStat))
        return S_ISLNK(fileStat.st_mode);

    return false;
}

bool DFile::isRegularFile(std::string file)
{
    if (file.empty())
        return false;

    struct stat fileStat;
    if (!stat(file.c_str(), &fileStat))
        return S_ISREG(fileStat.st_mode);

    return true;
}

bool DFile::isDir(std::string dir)
{
    if (dir.empty())
        return false;

    struct stat fileStat;
    if (!stat(dir.c_str(), &fileStat))
        return S_ISDIR(fileStat.st_mode);

    return false;
}

bool DFile::isExisted(std::string file)
{
    return !access(file.c_str(), F_OK);
}

std::string DFile::dir(std::string path)
{
    std::string ret;
    if (isDir(path)) {
        ret = path;
    } else {
        size_t pos = path.find_last_of("/");
        if (pos != std::string::npos) {
            ret.assign(path, 0, pos + 1); // 包含结尾斜杠/
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
