// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
