// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
