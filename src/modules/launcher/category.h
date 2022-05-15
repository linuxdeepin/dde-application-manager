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
