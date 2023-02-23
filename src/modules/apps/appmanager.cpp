// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appmanager.h"
#include "dfwatcher.h"
#include "alrecorder.h"
#include "basedir.h"
#include "dbusalrecorderadaptor.h"

#include <QDebug>
#include <QDir>

AppManager::AppManager(QObject *parent)
 : QObject(parent)
 , watcher(new DFWatcher(this))
 , recorder(new AlRecorder(watcher, this))
{
    qInfo() << "AppManager";
    new DBusAdaptorRecorder(recorder);
    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.registerService("org.deepin.dde.AlRecorder1"))
    {
        qWarning() << "register service AlRecorder1 error:" << con.lastError().message();
        return;
    }

    if (!con.registerObject("/org/deepin/dde/AlRecorder1", recorder))
    {
        qWarning() << "register object AlRecorder1 error:" << con.lastError().message();
        return;
    }


    QStringList dataDirs;
    dataDirs << BaseDir::userAppDir().c_str();
    for (auto &dir : BaseDir::sysAppDirs())
        dataDirs << dir.c_str();

    qInfo() << "get dataDirs: " << dataDirs;
    recorder->watchDirs(dataDirs);     // 监控应用desktop
}

AppManager::~AppManager()
{

}
