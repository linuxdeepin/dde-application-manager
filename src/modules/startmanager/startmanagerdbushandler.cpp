// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "startmanagerdbushandler.h"

#include <QDBusInterface>
#include <QDBusReply>

StartManagerDBusHandler::StartManagerDBusHandler(QObject *parent)
 : QObject(parent)
{

}

void StartManagerDBusHandler::markLaunched(QString desktopFile)
{
    QDBusInterface interface = QDBusInterface("org.deepin.dde.AlRecorder1", "/org/deepin/dde/AlRecorder1", "org.deepin.dde.AlRecorder1");
    interface.call("MarkLaunched", desktopFile);
}

QString StartManagerDBusHandler::getProxyMsg()
{
    QString ret;
    QDBusInterface interface = QDBusInterface("org.deepin.dde.NetworkProxy1", "/org/deepin/dde/NetworkProxy1", "org.deepin.dde.NetworkProxy1.App", QDBusConnection::systemBus());
    QDBusReply<QString> reply = interface.call("GetProxy");
    if (reply.isValid())
        ret = reply.value();

    return ret;
}

void StartManagerDBusHandler::addProxyProc(int32_t pid)
{
    QDBusInterface interface = QDBusInterface("org.deepin.dde.NetworkProxy1", "/org/deepin/dde/NetworkProxy1", "org.deepin.dde.NetworkProxy1.App", QDBusConnection::systemBus());
    interface.call("AddProc", pid);
}
