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

#ifndef APPMENU_H
#define APPMENU_H

#include <QString>
#include <QJsonObject>
#include <QVector>

#include <memory>
#include <vector>
#include <functional>

typedef std::function<void(uint32_t)> AppMenuAction;

class AppMenu;

// 应用菜单选项
struct AppMenuItem {
    QString id;
    QString text;
    bool isActive;
    QString isCheckable;
    QString checked;
    QString icon;
    QString iconHover;
    QString iconInactive;
    QString showCheckMark;
    std::shared_ptr<AppMenu> subMenu;

    int hint;
    AppMenuAction action;
};


// 应用菜单类
class AppMenu
{
public:
    AppMenu();

    void appendItem(AppMenuItem item);
    void handleAction(uint32_t timestamp, QString itemId);
    void setDirtyStatus(bool isDirty);
    QString getMenuJsonStr();

private:
    QString allocateId();

    QVector<AppMenuItem> items; // json:"items"
    bool checkableMenu; // json:"checkableMenu"
    bool singleCheck; // json:"singleCheck"

    int itemcount;
    bool dirty;
};

#endif // APPMENU_H
