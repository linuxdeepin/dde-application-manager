// SPDX-FileCopyrightText: 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "launcheriteminfo.h"

bool LauncherItemInfo::operator!=(const LauncherItemInfo &itemInfo)
{
    return itemInfo.path != path;
}

QDBusArgument &operator<<(QDBusArgument &argument, const LauncherItemInfo &itemInfo)
{
    argument.beginStructure();
    argument << itemInfo.path << itemInfo.name << itemInfo.id << itemInfo.icon << itemInfo.categoryId << itemInfo.timeInstalled << itemInfo.keywords;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, LauncherItemInfo &itemInfo)
{
    argument.beginStructure();
    argument >> itemInfo.path >> itemInfo.name >> itemInfo.id >> itemInfo.icon >> itemInfo.categoryId >> itemInfo.timeInstalled >> itemInfo.keywords;
    argument.endStructure();
    return argument;
}

void registerLauncherItemInfoMetaType()
{
    qRegisterMetaType<LauncherItemInfo>("ItemInfo");
    qDBusRegisterMetaType<LauncherItemInfo>();
}
