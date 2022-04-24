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

#include "dlocale.h"
#include "dstring.h"

#include <stdlib.h>
#include <pthread.h>

#define ComponentCodeset 1
#define ComponentTerritory 2
#define ComponentModifier 4

#define MAXLINELEN 256

const char *aliasFile = "/usr/share/locale/locale.alias";
const char charUscore = '_';
const char charDot = '.';
const char charAt = '@';

Locale::Locale()
{
    pthread_mutex_init(&languageNames.mutex, nullptr);

    // init aliases
    FILE *fp = fopen(aliasFile, "r");
    if (fp) {

    char data[MAXLINELEN] = {0};
    std::string line;
    std::vector<std::string> parts;
    while (fgets(data, MAXLINELEN, fp)) {
        char *start = &data[0];
        char *end = start;

        // 移除行首
        while (strneq(start, " ", 1) || strneq(start, "\t", 1))
            start++;

        // 过滤注释行和空行
        if (strneq(start, "#", 1) || strneq(start, "\n", 1))
            continue;

        while (!strneq(end, "\n", 1))
            end++;

        // 移除行尾
        while (strneq(end, "\n", 1) || strneq(end, "\r", 1)
            || strneq(end, " ", 1) || strneq(end, "\t", 1))
            end--;

        line.assign(start, ulong(end - start + 1));
        parts = DString::splitStr(line, ' ');
        // 使用\t分割
        if (parts.size() != 2)
            parts = DString::splitStr(line, '\t');

        if (parts.size() == 2) {
            aliases[parts[0]] = parts[1];
        }
    }
    fclose(fp);
    }
}

// wayland environment is useful?
// ExplodeLocale Break an X/Open style locale specification into components
Locale::Components Locale::explodeLocale(std::string locale)
{
    Components cmp;
    std::vector<std::string> parts;
    if (locale.find(charAt) != std::string::npos) {
        parts = DString::splitStr(locale, charAt);
        if (parts.size() == 2) {
            cmp.modifier = parts[1];
            locale = parts[0];
            cmp.mask |= ComponentModifier;
        }
    }

    if (locale.find(charDot) != std::string::npos) {
        parts = DString::splitStr(locale, charDot);
        if (parts.size() == 2) {
            cmp.codeset = parts[1];
            locale = locale[0];
            cmp.mask |= ComponentCodeset;
        }
    }

    if (locale.find(charUscore) != std::string::npos) {
        parts = DString::splitStr(locale, charUscore);
        if (parts.size() == 2) {
            cmp.territory = parts[1];
            locale = parts[0];
            cmp.mask |= ComponentTerritory;
        }
    }

    cmp.language = locale;
    return cmp;
}

std::string Locale::guessCategoryValue(std::string categoryName)
{
    // The highest priority value is the 'LANGUAGE' environment
	// variable.  This is a GNU extension.
    const char *language = getenv("LANGUAGE");
    if (language)
        return language;

    // Setting of LC_ALL overwrites all other.
    const char *lcAll = getenv("LC_ALL");
    if (lcAll)
        return lcAll;

    // Next comes the name of the desired category.
    const char *name = getenv(categoryName.c_str());
    if (name)
        return name;

    // Last possibility is the LANG environment variable.
    const char *lang = getenv("LANG");
    if (lang)
        return lang;

    return "C";
}

std::string Locale::unaliasLang(std::string lang)
{
    if (aliases.find(lang) != aliases.end())
        return aliases[lang];
    else
        return lang;
}

// wayland environment is useful?
/*
 * Compute all interesting variants for a given locale name -
 * by stripping off different components of the value.
 *
 * For simplicity, we assume that the locale is in
 * X/Open format: language[_territory][.codeset][@modifier]
 */
std::vector<std::string> Locale::getLocaleVariants(const std::string &locale)
{
    auto cmp = explodeLocale(locale);
    uint mask = cmp.mask;
    std::vector<std::string> variants;
    for (uint i = 0; i <= mask; i++) {
        uint j = mask - i;
        //if ((j & ^mask) == 0) {
            std::string var(cmp.language);
            if (j & ComponentTerritory)
                var = var + charUscore + cmp.territory;
            if (j & ComponentCodeset)
                var = var + charDot + cmp.codeset;
            if (j & ComponentModifier)
                var = var + charAt + cmp.modifier;

            variants.push_back(var);
        //}
    }

    return variants;
}

std::vector<std::string> Locale::getLanguageNames()
{
    std::vector<std::string> names;
    std::string value(guessCategoryValue("LC_MESSAGES"));
    if (value.empty()) {
        names.push_back(value);
        return names;
    }

    pthread_mutex_lock(&languageNames.mutex);
    if (languageNames.language != value) {
        languageNames.language = value;
        languageNames.names.clear();
        std::vector<std::string> langs = DString::splitStr(value, ':');
        for (const auto & lang : langs) {
            std::vector<std::string> localeVariant = getLocaleVariants(unaliasLang(lang));
            for (const auto & var : localeVariant)
                languageNames.names.push_back(var);
        }
        languageNames.names.push_back("C");
    }
    pthread_mutex_unlock(&languageNames.mutex);

    return languageNames.names;
}
