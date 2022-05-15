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

#ifndef STARTMANAGERDBUSHANDLER_H
#define STARTMANAGERDBUSHANDLER_H

#include <QObject>

class StartManagerDBusHandler : public QObject
{
    Q_OBJECT
public:
    explicit StartManagerDBusHandler(QObject *parent = nullptr);

    void markLaunched(QString desktopFile);

    QString getProxyMsg();
    void addProxyProc(int32_t pid);

Q_SIGNALS:

public slots:
};

#endif // STARTMANAGERDBUSHANDLER_H
