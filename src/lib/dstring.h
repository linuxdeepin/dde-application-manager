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
};

#endif // DSTRING_H
