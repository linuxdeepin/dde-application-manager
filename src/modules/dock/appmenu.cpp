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

#include "appmenu.h"

#include <QJsonArray>
#include <QJsonDocument>

AppMenu::AppMenu()
 : checkableMenu(false)
 , singleCheck(false)
 , itemcount(0)
 , dirty(false)
{

}

// 增加菜单选项
void AppMenu::appendItem(AppMenuItem item)
{
    if (!item.text.isEmpty()) {
        item.id = allocateId();
        items.push_back(item);
    }
}

void AppMenu::handleAction(uint32_t timestamp, QString itemId)
{
    for (auto &item : items) {
        if (item.id == itemId) {
            item.action(timestamp);
            break;
        }
    }
}

void AppMenu::setDirtyStatus(bool isDirty)
{
    dirty = isDirty;
}

QString AppMenu::getMenuJsonStr()
{
    QJsonObject obj;
    QJsonArray array;
    for (auto item : items) {
        QJsonObject objItem;
        objItem["itemId"] = item.id;
        objItem["itemText"] = item.text;
        objItem["isActive"] = item.isActive;
        objItem["isCheckable"] = item.isCheckable;
        objItem["checked"] = item.checked;
        objItem["itemIcon"] = item.icon;
        objItem["itemIconHover"] = item.iconHover;
        objItem["itemIconInactive"] = item.iconInactive;
        objItem["showCheckMark"] = item.showCheckMark;
        objItem["itemSubMenu"] = item.subMenu ? item.subMenu->getMenuJsonStr() : "";
        array.push_back(QJsonValue(objItem));
    }
    obj["items"] = QJsonValue(array);
    obj["checkableMenu"] = checkableMenu;
    obj["singleCheck"] = singleCheck;

    QString ret = QJsonDocument(obj).toJson();
    return ret;
}

QString AppMenu::allocateId()
{
    return QString::number(itemcount++);
}


