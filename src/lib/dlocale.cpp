// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    pthread_mutex_init(&m_languageNames.mutex, nullptr);

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
            m_aliases[parts[0]] = parts[1];
        }
    }
    fclose(fp);
    }
}

/**
 * @brief Locale::explodeLocale 拆分locale
 * @param locale
 * @return
 */
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
    if (m_aliases.find(lang) != m_aliases.end())
        return m_aliases[lang];
    else
        return lang;
}

/**
 * @brief Locale::getLocaleVariants
 * @param locale
 * @return
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

    pthread_mutex_lock(&m_languageNames.mutex);
    if (m_languageNames.language != value) {
        m_languageNames.language = value;
        m_languageNames.names.clear();
        std::vector<std::string> langs = DString::splitStr(value, ':');
        for (const auto & lang : langs) {
            std::vector<std::string> localeVariant = getLocaleVariants(unaliasLang(lang));
            for (const auto & var : localeVariant)
                m_languageNames.names.push_back(var);
        }
        m_languageNames.names.push_back("C");
    }
    pthread_mutex_unlock(&m_languageNames.mutex);

    return m_languageNames.names;
}
