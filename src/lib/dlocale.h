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

#ifndef LOCALE_H
#define LOCALE_H

#include <string>
#include <vector>
#include <map>

// 本地化类
class Locale {
    struct LanguageNameCache {
        std::string language;
        std::vector<std::string> names;
        pthread_mutex_t mutex;
    };

    struct Components {
        Components() : mask(0) {}   // 数字必须初始化
        std::string language;
        std::string territory;
        std::string codeset;
        std::string modifier;
        uint mask;
    };

public:
    std::vector<std::string> getLocaleVariants(const std::string &locale);
    std::vector<std::string> getLanguageNames();
    static inline Locale *instance() {
        static Locale instance;
        return &instance;
    }

private:
    Locale();
    Locale(const Locale &);
    Locale& operator= (const Locale &);

    Components explodeLocale(std::string locale);
    std::string guessCategoryValue(std::string categoryName);
    std::string unaliasLang(std::string);
    std::map<std::string, std::string> m_aliases;
    LanguageNameCache m_languageNames;
};

#endif
