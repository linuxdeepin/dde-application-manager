// SPDX-FileCopyrightText: 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "launcheriteminfo.h"

#include <QtCore/QList>
#include <QDBusMetaType>

typedef QList<LauncherItemInfo> LauncherItemInfoList;

Q_DECLARE_METATYPE(LauncherItemInfoList)

void registerLauncherItemInfoListMetaType();
