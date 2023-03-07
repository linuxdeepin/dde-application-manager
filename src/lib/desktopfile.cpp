// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktopfile.h"

DesktopFile::DesktopFile(char separtor)
 : KeyFile(separtor)
{
}

DesktopFile::~DesktopFile() = default;

bool DesktopFile::saveToFile(const std::string &filePath){
    FILE *sfp = fopen(filePath.data(), "w+");
    if (!sfp) {
        perror("open file failed...");
        return false;
    }

    // NOTE(black_desk): XDG require the first section of desktop file
    // is always "Desktop Entry"

    auto mainSection = m_mainKeyMap.find("Desktop Entry");
    if (mainSection != m_mainKeyMap.end()) {
        // FIXME(black_desk): should handle write fail.
        writeSectionToFile(mainSection->first, mainSection->second, sfp);
    } else {
        // FIXME(black_desk): should have some warning.
    }

    for (const auto &im : m_mainKeyMap) {
        if (im.first == "Desktop Entry") {
            continue;
        }
        // FIXME(black_desk): should handle write fail.
        writeSectionToFile(im.first, im.second, sfp);
    }

    fclose(sfp);
    return true;
}
