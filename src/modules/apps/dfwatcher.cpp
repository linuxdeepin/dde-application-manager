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

#include "dfwatcher.h"

#include <QDir>
#include <QDBusConnection>
#include <QDBusError>
#include <QtDebug>

const QString dfSuffix = ".desktop";
const QString configSuffix = ".json";

 DFWatcher::DFWatcher(QObject *parent)
  : QObject (parent)
  , watcher(new QFileSystemWatcher(this))
 {
     QDBusConnection con = QDBusConnection::sessionBus();
     if (!con.registerService("org.deepin.daemon.DFWatcher1"))
     {
         qInfo() << "register service app1 error:" << con.lastError().message();
         return;
     }

     if (!con.registerObject("/org/deepin/daemon/DFWatcher1", this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals))
     {
         qInfo() << "register object DFWatcher error:" << con.lastError().message();
         return;
     }

     connect(watcher, &QFileSystemWatcher::fileChanged, this, &DFWatcher::fileUpdated, Qt::QueuedConnection);
     connect(watcher, &QFileSystemWatcher::directoryChanged, this, &DFWatcher::directoryUpdated, Qt::QueuedConnection);
 }

 DFWatcher::~DFWatcher()
 {
     QDBusConnection::sessionBus().unregisterObject("/org/deepin/daemon/DFWatcher1");
 }

 void DFWatcher::addDir(const QString &path)
 {
    // 记录当前目录内容
    qInfo() << "addDir :" << path;
    const QDir dir(path);
    dirContentMap[path] = dir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDir::DirsFirst);

    watcher->addPath(path);
 }

 void DFWatcher::addPaths(const QStringList &paths)
 {
    watcher->addPaths(paths);
 }

 QStringList DFWatcher::files()
 {
     return watcher->files();
 }

 void DFWatcher::removePath(const QString &filepath)
 {
    watcher->removePath(filepath);
 }

 void DFWatcher::fileUpdated(const QString &filePath)
 {
     qInfo() << "event: modify filepath=" << filePath;
     if (filePath.endsWith(dfSuffix) || filePath.endsWith(configSuffix))
        Q_EMIT Event(filePath, int(event::Mod));
 }

 void DFWatcher::directoryUpdated(const QString &dirPath)
 {
    QStringList recoderedContent = dirContentMap[dirPath];

    const QDir dir(dirPath);
    QStringList newEntries = dir.entryList(QDir::NoDotAndDotDot  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);

    QSet<QString> newDirSet = QSet<QString>::fromList(newEntries);
    QSet<QString> currentDirSet = QSet<QString>::fromList(recoderedContent);

    // 添加的文件
    QSet<QString> newFiles = newDirSet - currentDirSet;
    QStringList newFile = newFiles.toList();

    // 移除的文件
    QSet<QString> deletedFiles = currentDirSet - newDirSet;
    QStringList deleteFile = deletedFiles.toList();

    // 更新目录记录
    dirContentMap[dirPath] = newEntries;

    // 新增文件
    if (newFile.size() > 0) {
        for (auto &file : newFile) {
            //qInfo() << "event: add filepath=" << file;
            if (file.endsWith(dfSuffix)) {
                Q_EMIT Event(dirPath + file, event::Add);
            }
        }
    }

    // 移除文件
    if (deleteFile.size() > 0) {
        for (auto &file : deleteFile) {
            //qInfo() << "event: del filepath=" << file;
            if (file.endsWith(dfSuffix)) {
                Q_EMIT Event(dirPath + file, event::Del);
            }
        }
    }
 }


