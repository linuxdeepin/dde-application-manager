// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TYPES_H
#define TYPES_H

#include <QMap>
#include <QStringList>
#include <QDBusMetaType>

typedef QMap<QString, QStringList> UnLaunchedAppMap;

Q_DECLARE_METATYPE(UnLaunchedAppMap)

void registerUnLaunchedAppMapMetaType();

#endif // TYPES_H
