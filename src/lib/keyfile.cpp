// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "keyfile.h"
#include "dlocale.h"
#include "dstring.h"
#include "macro.h"

#include <cstring>
#include <string>
#include <iostream>

KeyFile::KeyFile(char separtor)
 : m_fp(nullptr)
 , m_modified(false)
 , m_listSeparator(separtor)
{
}

KeyFile::~KeyFile()
{
    if (m_fp) {
        fclose(m_fp);
        m_fp = nullptr;
    }
}

bool KeyFile::getBool(const std::string &section, const std::string &key, bool defaultValue)
{
    if (m_mainKeyMap.find(section) == m_mainKeyMap.end())
        return false;

    std::string valueStr = m_mainKeyMap[section][key];
    bool value = defaultValue;
    if (valueStr == "true")
        value = true;
    else if (valueStr == "false")
        value = false;

    return value;
}

void KeyFile::setBool(const std::string &section, const std::string &key, const std::string &defaultValue)
{
    if (m_mainKeyMap.find(section) == m_mainKeyMap.end())
        m_mainKeyMap.insert({section, KeyMap()});

    m_mainKeyMap[section][key] = defaultValue;
}

// TODO
std::vector<bool> KeyFile::getBoolList(const std::string &section, const std::string &key, bool defaultValue)
{
    std::vector<bool> tmp;
    return tmp;
}

int KeyFile::getInt(const std::string &section, const std::string &key, int defaultValue)
{
    if (m_mainKeyMap.find(section) == m_mainKeyMap.end())
        return defaultValue;

    std::string valueStr = m_mainKeyMap[section][key];
    int value;
    try {
        value = std::stoi(valueStr);
    } catch (std::invalid_argument&) {
        value = defaultValue;
    }

    return value;
}

// TODO
std::vector<int> KeyFile::getIntList(const std::string &section, const std::string &key, int defaultValue)
{
    std::vector<int> tmp;
    return tmp;
}

// TODO
int64_t KeyFile::getInt64(const std::string &section, const std::string &key, int64_t defaultValue)
{
    return int64_t(0);
}

// TODO
uint64_t KeyFile::getUint64(const std::string &section, const std::string &key, int64_t defaultValue)
{
    return uint64_t(0);
}

// TODO
float KeyFile::getFloat(const std::string &section, const std::string &key, float defaultValue)
{
    return 1.0;
}

std::string KeyFile::getStr(const std::string &section, const std::string &key, std::string defaultValue)
{
    if (m_mainKeyMap.find(section) == m_mainKeyMap.end())
        return defaultValue;

    std::string valueStr = m_mainKeyMap[section][key];
    if (valueStr.empty())
        valueStr = defaultValue;

    return valueStr;
}

bool KeyFile::containKey(const std::string &section, const std::string &key)
{
    if (m_mainKeyMap.find(section) == m_mainKeyMap.end())
        return false;

    return m_mainKeyMap[section].find(key) != m_mainKeyMap[section].end();
}

std::string KeyFile::getLocaleStr(const std::string &section, const std::string &key, std::string defaultLocale)
{
    std::vector<std::string> languages = defaultLocale.empty()
     ? Locale::instance()->getLanguageNames()
     : Locale::instance()->getLocaleVariants(defaultLocale);

    std::string translated;
    for (const auto &lang : languages) {
        translated.assign(getStr(section, key + "[" + lang + "]"));
        if (!translated.empty())
            return translated;
    }

    // NOTE: not support key Gettext-Domain
	// fallback to default key
	return getStr(section, key);
}

std::vector<std::string> KeyFile::getStrList(const std::string &section, const std::string &key)
{
    std::string value = getStr(section, key);
    return DString::splitStr(value, m_listSeparator);
}

std::vector<std::string> KeyFile::getLocaleStrList(const std::string &section, const std::string &key, std::string defaultLocale)
{
    std::vector<std::string> languages = defaultLocale.empty()
     ? Locale::instance()->getLanguageNames()
     : Locale::instance()->getLocaleVariants(defaultLocale);

    std::vector<std::string> translated;
    for (const auto &lang : languages) {
        translated = getStrList(section, key + "[" + lang + "]");
        if (translated.size() > 0)
            return translated;
    }

    //fallback to default key
    return getStrList(section, key);
}

// 修改keyfile内容
void KeyFile::setKey(const std::string &section, const std::string &key, const std::string &value)
{
    if (m_mainKeyMap.find(section) == m_mainKeyMap.end())
        m_mainKeyMap.insert({section, KeyMap()});

    m_mainKeyMap[section][key] = value;
}

// 写入文件
bool KeyFile::saveToFile(const std::string &filePath)
{
    FILE *sfp = fopen(filePath.data(), "w+");
    if (!sfp) {
        perror("open file failed...");
        return false;
    }

    for (const auto &im : m_mainKeyMap) {
        const auto &keyMap = im.second;
        std::string section = "[" + im.first + "]\n";
        fputs(section.c_str(), sfp);
        for (const auto &ik : keyMap) {
            std::string kv = ik.first + "=" + ik.second + "\n";
            fputs(kv.c_str(), sfp);
        }
    }

    fclose(sfp);
    return true;
}

bool KeyFile::loadFile(const std::string &filePath)
{
    m_mainKeyMap.clear();
    if (m_fp) {
        fclose(m_fp);
        m_fp = nullptr;
    }

    std::string lastSection;
    m_fp = fopen(filePath.data(), "r");
    if (!m_fp) {
        perror("open file failed: ");
        return false;
    }

    char line[MAX_LINE_LEN] = {0};
    while (fgets(line, MAX_LINE_LEN, m_fp)) {
        char *start = &line[0];
        char *end = start;
        while (!strneq(end, "\0", 1))
            end++;

        end--; // 返回'\0'前一个字符

        // 移除行首
        while (strneq(start, " ", 1) || strneq(start, "\t", 1))
            start++;

        // 过滤注释行
        if (strneq(start, "#", 1))
            continue;

        // 移除行尾
        while (strneq(end, "\n", 1) || strneq(end, "\r", 1)
            || strneq(end, " ", 1) || strneq(end, "\t", 1))
            end--;

        char *lPos = strchr(start, '[');
        char *rPos = strchr(start, ']');
        if (lPos && rPos && rPos - lPos > 0 && lPos == start && rPos == end) {
            // 主键
            std::string section(lPos + 1, size_t(rPos - lPos - 1));
            m_mainKeyMap.insert({section, KeyMap()});
            lastSection = section;
        } else {
            char *equal = strchr(start, '=');
            if (!equal)
                continue;

            // 文件格式错误
            if (lastSection.empty()) {
                std::cout << "failed to load file " << filePath << std::endl;
                return false;
            }

            // 子键
            std::string key(start, size_t(equal - start));
            std::string value(equal + 1, size_t(end - equal));
            for (auto &iter : m_mainKeyMap) {
                if (iter.first != lastSection)
                    continue;

                iter.second[key] = value;
            }
        }
    }
    fclose(m_fp);
    m_fp = nullptr;
    m_filePath = filePath;

    return true;
}

std::vector<std::string> KeyFile::getMainKeys()
{
    std::vector<std::string> mainKeys;
    for (const auto &iter : m_mainKeyMap)
        mainKeys.push_back(iter.first);

    return mainKeys;
}

void KeyFile::print()
{
    std::cout << "sectionMap: " << std::endl;
    for (auto sectionMap : m_mainKeyMap) {
        std::cout << "section=" << sectionMap.first << std::endl;
        KeyMap keyMap = sectionMap.second;

        for (auto iter : keyMap) {
            std::cout << iter.first << "=" << iter.second << std::endl;
        }

        std::cout << std::endl;
    }
}
