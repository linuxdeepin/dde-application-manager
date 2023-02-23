// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KEYFILE_H
#define KEYFILE_H

#include <string>
#include <map>
#include <vector>

typedef std::map<std::string, std::string> KeyMap;
typedef std::map<std::string, KeyMap> MainKeyMap;

// 解析ini、desktop文件类
class KeyFile
{
public:
    explicit KeyFile(char separtor = ';');
    ~KeyFile();

    bool getBool(const std::string &section, const std::string &key, bool defaultValue = false);
    void setBool(const std::string &section, const std::string &key, const std::string &defaultValue = "false");
    std::vector<bool> getBoolList(const std::string &section, const std::string &key, bool defaultValue = false);
    int getInt(const std::string &section, const std::string &key, int defaultValue = 0);
    std::vector<int> getIntList(const std::string &section, const std::string &key, int defaultValue = 0);
    int64_t getInt64(const std::string &section, const std::string &key, int64_t defaultValue = 0);
    uint64_t getUint64(const std::string &section, const std::string &key, int64_t defaultValue = 0);
    float getFloat(const std::string &section, const std::string &key, float defaultValue = 0);
    std::string getStr(const std::string &section, const std::string &key, std::string defaultValue = "");
    bool containKey(const std::string &section, const std::string &key);
    std::string getLocaleStr(const std::string &section, const std::string &key, std::string defaultLocale = "");
    std::vector<std::string> getStrList(const std::string &section, const std::string &key);
    std::vector<std::string> getLocaleStrList(const std::string &section, const std::string &key, std::string defaultLocale = "");

    void setKey(const std::string &section, const std::string &key, const std::string &value);
    bool saveToFile(const std::string &filePath);
    bool loadFile(const std::string &filePath);
    std::vector<std::string> getMainKeys();
    std::string getFilePath()
    {
        return m_filePath;
    }

    // for test
    void print();

private:
    MainKeyMap m_mainKeyMap; // section -> key : value
    std::string m_filePath;
    FILE *m_fp;
    bool m_modified;
    char m_listSeparator;
};

#endif // KEYFILE_H
