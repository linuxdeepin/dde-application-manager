// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DESKTOPFILE_H
#define DESKTOPFILE_H

#include "keyfile.h"

#include <string>
#include <map>
#include <vector>

// 解析desktop文件类
class DesktopFile: public KeyFile
{
public:
    explicit DesktopFile(char separtor = ';');
    virtual ~DesktopFile();

    virtual bool saveToFile(const std::string &filePath) override;
};

#endif // DESKTOPFILE_H
