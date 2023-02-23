// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILE_H
#define DFILE_H

#include <string>

class DFile
{
public:
    explicit DFile();
    static bool isLink(std::string file);
    static bool isRegularFile(std::string file);
    static bool isDir(std::string dir);
    static bool isExisted(std::string file);
    static std::string dir(std::string path);
    static std::string base(std::string file);
};

#endif // DFILE_H
