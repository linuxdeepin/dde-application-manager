// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "keyfile.h"
#include "dlocale.h"
#include "dstring.h"
#include "macro.h"

#include <cstring>
#include <fstream>
#include <string>
#include <iostream>
#include <algorithm>

KeyFile::KeyFile(char separtor)
 : m_modified(false)
 , m_listSeparator(separtor)
{
}

KeyFile::~KeyFile()
{
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
        if (!writeSectionToFile(im.first, im.second,sfp)){
            return false;
        }
    }

    fclose(sfp);
    return true;
}

bool KeyFile::writeSectionToFile(const std::string& sectionName, const KeyMap& keyMap, FILE * file){
    if (file == nullptr) {
        return false;
    }

    std::string section = "[" + sectionName + "]\n";
    fputs(section.c_str(), file);
    for (const auto &ik : keyMap) {
        std::string kv = ik.first + "=" + ik.second + "\n";
        fputs(kv.c_str(), file);
    }

    // FIXME(black_desk): should handle fputs error
    return true;
}

bool KeyFile::loadFile(const std::string &filePath)
{
    m_mainKeyMap.clear();

    std::string lastSection;
    std::ifstream fs(filePath);
    if (!fs.is_open()) {
        perror("open file failed: ");
        return false;
    }

    std::string line;
    while (std::getline(fs, line)) {
        line.erase(
            line.begin(),
            std::find_if(
                line.begin(), line.end(),
                std::not1(std::ptr_fun<int, int>(std::isspace))
            )
        );

        if (line.empty() || line.front() == '#') {
            continue;
        }

        line.erase(
            std::find_if(
                line.rbegin(), line.rend(),
                std::not1(std::ptr_fun<int, int>(std::isspace))
            ).base(),
            line.end()
        );

        if (line.empty()) {
            continue;
        }

        if (line.front() == '[') {
            auto rPos = line.find_first_of(']');
            if ( rPos != std::string::npos && 0 < rPos ) {
                // TODO(black_desk): lastSection might be empty string here.
                // I cannot found a spec for the space before groupname in
                // freedesktop.org
                lastSection = line.substr(1, rPos-1);

                m_mainKeyMap.insert({
                    lastSection,
                    KeyMap()
                });

            }
            continue;
        }


        auto equalPos = line.find_first_of('=');

        if (equalPos == std::string::npos) {
            continue;
        }

        // 文件格式错误
        if (lastSection.empty()) {
            std::cout << "failed to load file " << filePath << std::endl;
            return false;
        }

        // 子键

        // TODO(black_desk): we should check chars in key here, as spec says
        // that it can only contain a-z0-9A-Z
        std::string key = line.substr(0, equalPos);
        std::string value = equalPos + 1 < line.length() ?
            line.substr(equalPos + 1) :
            "";

        // TODO(black_desk): space after value is removed before. But I cannot
        // find a spec about this behavior.
        m_mainKeyMap[lastSection][key] = value;
    }

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
