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

#include "startmanagerdbushandler.h"

#include <QDBusInterface>
#include <QDBusReply>

StartManagerDBusHandler::StartManagerDBusHandler(QObject *parent)
 : QObject(parent)
{

}

void StartManagerDBusHandler::markLaunched(QString desktopFile)
{
    QDBusInterface interface = QDBusInterface("org.deepin.daemon.AlRecoder1", "/org/deepin/daemon/AlRecoder1", "org.deepin.daemon.AlRecoder1");
    interface.call("MarkLaunched", desktopFile);
}

QString StartManagerDBusHandler::getProxyMsg()
{
    QString ret;
    QDBusInterface interface = QDBusInterface("com.deepin.system.proxy", "/com/deepin/system/proxy", "com.deepin.system.proxy.App");
    QDBusReply<QString> reply = interface.call("GetProxy");
    if (reply.isValid())
        ret = reply.value();

    return ret;
}

void StartManagerDBusHandler::addProxyProc(int32_t pid)
{
    QDBusInterface interface = QDBusInterface("com.deepin.system.proxy", "/com/deepin/system/proxy", "com.deepin.system.proxy.App");
    interface.call("AddProc");
}


