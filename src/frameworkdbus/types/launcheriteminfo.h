// SPDX-FileCopyrightText: 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtCore/QString>
#include <QtDBus/QtDBus>
#include <QtCore/QList>
#include <QDBusMetaType>
#include <QDebug>

struct LauncherItemInfo {
    QString path;
    QString name;
    QString id;
    QString icon;
    qint64 categoryId;
    qint64 timeInstalled;
    QStringList keywords;
    bool operator!=(const LauncherItemInfo &versionInfo);

    friend QDebug operator<<(QDebug argument, const LauncherItemInfo &info)
    {
        argument << info.path << info.name << info.id;
        argument << info.icon << info.categoryId << info.timeInstalled << info.keywords;

        return argument;
    }
};

Q_DECLARE_METATYPE(LauncherItemInfo)

QDBusArgument &operator<<(QDBusArgument &argument, const LauncherItemInfo &versionInfo);
const QDBusArgument &operator>>(const QDBusArgument &argument, LauncherItemInfo &versionInfo);
void registerLauncherItemInfoMetaType();
