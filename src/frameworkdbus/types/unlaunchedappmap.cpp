// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "unlaunchedappmap.h"

void registerUnLaunchedAppMapMetaType()
{
    qRegisterMetaType<UnLaunchedAppMap>("UnLaunchedAppMap");
    qDBusRegisterMetaType<UnLaunchedAppMap>();
}
