// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSTRING_H
#define DSTRING_H

#include <vector>
#include <string>
#include <cstring>

#define streq(a,b) (strcmp((a),(b)) == 0)
#define strneq(a, b, n) (strncmp((a), (b), (n)) == 0)
#define strcaseeq(a,b) (strcasecmp((a),(b)) == 0)
#define strncaseeq(a, b, n) (strncasecmp((a), (b), (n)) == 0)

// 字符串操作
class DString
{
public:
    DString();
    ~DString();

    // 字符串拆分
    static std::vector<std::string> splitChars(const char *cs, char c);
    static std::vector<std::string> splitStr(const std::string &str, char c);
    static std::vector<std::string> splitVectorChars(const std::vector<char> &content, size_t length, char c);
    // 字符串前缀判断
    static bool startWith(const char *chars, const char *prefix);
    static bool startWith(const std::string &str, const std::string &prefix);
    // 字符后缀判断
    static bool endWith(const char *chars, const char *suffix);
    static bool endWith(const std::string &str, const std::string &suffix);
    // 去除首尾引用
    static char *delQuote(const char *chars);
    static void delQuote(std::string &str);
    // 连接字符串
    static std::string join(std::vector<std::string> strs, std::string joinStr);
};

#endif // DSTRING_H
