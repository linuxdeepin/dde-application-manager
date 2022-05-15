/*
 * Copyright (C) 2021 ~ 2022 Deepin Technology Co., Ltd.
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

#ifndef LANG_H
#define LANG_H

#include "dstring.h"

#include <stdlib.h>
#include <cstring>
#include <string>
#include <vector>
#include <array>

// 返回用户语言,参见man gettext
inline std::vector<std::string> queryLangs() {
    std::vector<std::string> ret;
    const char *lcAll = getenv("LC_ALL");
    const char *lcMessage = getenv("LC_MESSAGE");
    const char *language = getenv("LANGUAGE");
    const char *lang = getenv("LANG");

    auto cutOff = [](std::string str)->std::string {
        size_t idx = str.find(".");
        if (idx == std::string::npos)
            return str;

        return std::string(str).substr(0, idx);
    };

    if (lcAll && std::string(lcAll) != "C"
        && language && std::string(language) != "")
    {
        std::vector<std::string> splits = DString::splitChars(language, ':');
        for (const auto &l : splits) {
            ret.push_back(cutOff(l));
        }
        return ret;
    }

    if (lcAll && std::string(lcAll) != "")
        ret.push_back(cutOff(lcAll));

    if (lcMessage && std::string(lcMessage) != "")
        ret.push_back(cutOff(lcMessage));

    if (lang && std::string(lang) != "")
        ret.push_back(cutOff(lang));

    return ret;
}


#endif // LANG_H
