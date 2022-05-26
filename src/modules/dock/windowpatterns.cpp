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

#include "windowpatterns.h"
#include "common.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QVariant>
#include <QVariantMap>
#include <QDebug>
#include <dstring.h>
#include <QRegExp>
#include <QFile>

const int parsedFlagNegative = 0x001;
const int parsedFlagIgnoreCase = 0x010;

bool contains(QString key, QString value) {
    return key.contains(value);
}

bool containsIgnoreCase(QString key, QString value) {
    QString _key = key.toLower();
    QString _value = value.toLower();
    return _key.contains(_value);
}

bool equal(QString key, QString value) {
    return key == value;
}

bool equalIgnoreCase(QString key, QString value) {
    return key.toLower() == value.toLower();
}

bool regexMatch(QString key, QString value) {
    QRegExp ruleRegex(value);
    return ruleRegex.exactMatch(key);
}

bool regexMatchIgnoreCase(QString key, QString value) {
    QRegExp ruleRegex(value, Qt::CaseInsensitive);
    return ruleRegex.exactMatch(key);
}


RuleValueParse::RuleValueParse()
 : negative(0)
 , type(0)
 , flags(0)
{
}

bool RuleValueParse::parse(QString parsedKey)
{
    if (!fn)
        return false;

    return negative ? fn(parsedKey, value) : !fn(parsedKey, value);
}

bool RuleValueParse::match(const WindowInfoX *winInfo)
{
    QString parsedKey;

    return parse(parsedKey);
}


WindowPatterns::WindowPatterns()
{

}

/**
 * @brief WindowPatterns::match 匹配窗口
 * @param winInfo
 * @return
 */
QString WindowPatterns::match(WindowInfoX *winInfo)
{
    for (auto pattern : patterns) {

    }
    return "";
}

void WindowPatterns::loadWindowPatterns()
{
    qInfo() << "---loadWindowPatterns";
    QFile file(windowPatternsFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

     QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
     file.close();
     if (!doc.isArray())
         return;

     QJsonArray arr = doc.array();
     if (arr.size() == 0)
         return;

     patterns.clear();
     for (auto iterp = arr.begin(); iterp != arr.end(); iterp++) {
         // 过滤非Object
        if (!(*iterp).isObject())
            continue;

        QJsonObject patternObj = (*iterp).toObject();
        QVariantMap patternMap = patternObj.toVariantMap();
        WindowPattern pattern;
        for (auto infoIter = patternMap.begin(); infoIter != patternMap.end(); infoIter++) {
            QString ret = infoIter.key();
            QVariant value = infoIter.value();

            if (ret == "ret") {
                pattern.result = value.toString();
            } else if (ret == "rules") {
                for (auto &item : value.toList()) {
                    if (!item.isValid())
                        continue;

                    if (item.toList().size() != 2)
                        continue;

                    pattern.rules.push_back({item.toList()[0].toString(), item.toList()[1].toString()});
                }
            }
        }
        qInfo() << pattern.result;
        for (const auto &item : pattern.rules) {
            qInfo() << item[0] << " " << item[1];
        }
        patterns.push_back(pattern);
     }

     // 解析patterns
     for (auto &pattern : patterns) {
        for (int i=0; i < pattern.rules.size(); i++) {
            RuleValueParse ruleValue = parseRule(pattern.rules[i]);
            pattern.parseRules.push_back(ruleValue);
        }
     }
}

// "=:XXX" equal XXX
// "=!XXX" not equal XXX

// "c:XXX" contains XXX
// "c!XXX" not contains XXX

// "r:XXX" match regexp XXX
// "r!XXX" not match regexp XXX

// e c r ignore case
// = E C R not ignore case
// 解析窗口类型规则
RuleValueParse WindowPatterns::parseRule(QVector<QString> rule)
{
    RuleValueParse ret;
    ret.key = rule[0];
    ret.original = rule[1];
    if (rule[1].size() < 2)
        return ret;

    int len = ret.original.size() + 1;
    char *orig = static_cast<char *>(calloc(1, size_t(len)));
    if (!orig)
        return ret;

    strncpy(orig, ret.original.toStdString().c_str(), size_t(len));
    switch (orig[1]) {
    case ':':
    case '!':
        ret.flags |= parsedFlagNegative;
        ret.negative = true;
        break;
    default:
        return ret;
    }

    ret.value = QString(&orig[2]);
    ret.type = uint8_t(orig[0]);
    switch (orig[0]) {
    case 'C':
        ret.fn = contains;
        break;
    case 'c':
        ret.flags |= parsedFlagIgnoreCase;
        ret.fn = containsIgnoreCase;
        break;
    case '=':
    case 'E':
        ret.fn = equal;
        break;
    case 'e':
        ret.flags |= parsedFlagIgnoreCase;
        ret.fn = equalIgnoreCase;
        break;
    case 'R':
        ret.fn = regexMatch;
        break;
    case 'r':
        ret.flags |= parsedFlagIgnoreCase;
        ret.fn = regexMatchIgnoreCase;
        break;
    default:
        return ret;
    }

    return ret;
}


