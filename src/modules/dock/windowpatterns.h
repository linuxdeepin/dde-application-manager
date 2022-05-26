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

#ifndef WINDOWPATTERNS_H
#define WINDOWPATTERNS_H

#include "windowinfox.h"

#include <QString>
#include <QVector>


struct RuleValueParse {
    RuleValueParse();
    bool parse(QString parsedKey);
    bool match(const WindowInfoX *winInfo);
    QString key;
    bool negative;
    bool (*fn)(QString, QString);
    uint8_t type;
    uint flags;
    QString original;
    QString value;
};

class WindowPatterns
{
    // 窗口类型匹配
    struct WindowPattern {
        QVector<QVector<QString>> rules;    // rules
        QString result;                     // ret
        QVector<RuleValueParse> parseRules;
    };

public:
    WindowPatterns();

    QString match(WindowInfoX *winInfo);

private:
    void loadWindowPatterns();
    RuleValueParse parseRule(QVector<QString> rule);
    QVector<WindowPattern> patterns;

};

#endif // WINDOWPATTERNS_H
