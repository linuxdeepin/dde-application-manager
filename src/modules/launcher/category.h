// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CATEGORY_H
#define CATEGORY_H

#include "common.h"

#include <QString>
#include <QMultiMap>

// 应用类型
enum class Categorytype {
    CategoryInternet,
    CategoryChat,
    CategoryMusic,
    CategoryVideo,
    CategoryGraphics,
    CategoryGame,
    CategoryOffice,
    CategoryReading,
    CategoryDevelopment,
    CategorySystem,
    CategoryOthers,
    CategoryErr,
};

class Category
{
public:
    Category();
    ~Category();

    // 类型转字符串
    static QString getStr(Categorytype ty);
    // 类型转拼音
    static QString pinYin(Categorytype ty);
    // 字符串转类型
    static Categorytype parseCategoryString(QString str);
    // Xorg类型字符串转类型列表
    static QList<Categorytype> parseXCategoryString(QString str);
};

#endif // CATEGORY_H
