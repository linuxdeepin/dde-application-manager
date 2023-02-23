// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "alrecorder.h"
#include "dfwatcher.h"

#include <QDir>
#include <QDBusConnection>
#include <QDBusError>
#include <QtDebug>
#include <QCryptographicHash>

const QString userAppsCfgDir = QDir::homePath() + "/.config/deepin/dde-daemon/apps/";

AlRecorder::AlRecorder(DFWatcher *_watcher, QObject *parent)
 : QObject (parent)
 , watcher(_watcher)
 , mutex(QMutex(QMutex::NonRecursive))
{
    connect(watcher, &DFWatcher::Event, this, &AlRecorder::onDFChanged, Qt::QueuedConnection);
    Q_EMIT serviceRestarted();
}

AlRecorder::~AlRecorder()
{
}

/**
 * @brief AlRecorder::getNew 获取未启动应用列表
 * @return
 */
QMap<QString, QStringList> AlRecorder::getNew()
{
    QMap<QString, QStringList> ret;
    QMutexLocker locker(&mutex);
    for (auto is = subRecoders.begin(); is != subRecoders.end(); is++) {
        QStringList apps;
        for (auto il = is.value().launchedMap.begin(); il != is.value().launchedMap.end(); il++) {
            if (!il.value()) // 未启动应用
                apps.push_back(il.key());
        }

        if (apps.size() > 0)
            ret[is.key()] = apps;
    }

    return ret;
}

/**
 * @brief AlRecorder::markLaunched 标记应用已启动状态
 * @param filePath
 */
void AlRecorder::markLaunched(const QString &filePath)
{
    if (!filePath.endsWith(".desktop"))
        return;

    QFileInfo info;
    QMutexLocker locker(&mutex);
    for (auto sri = subRecoders.begin(); sri != subRecoders.end(); sri++) {
        if (!filePath.startsWith(sri.key()))
            continue;

        info.setFile(filePath);
        QString name = info.baseName();
        for (auto li = sri.value().launchedMap.begin(); li != sri.value().launchedMap.end(); li++) {
            // 查找同名且未启动过的应用
            if (li.key() == name && !li.value()) {
                li.value() = true;
                Q_EMIT launched(filePath);

                // 记录启动状态
                saveStatusFile(info.absolutePath() + "/");
            }
        }
    }
}

/**
 * @brief AlRecorder::uninstallHints 记录Launcher服务卸载应用信息, 终端卸载不会调用该函数， desktopFiles为绝对路径
 * @param desktopFiles
 */
void AlRecorder::uninstallHints(const QStringList &desktopFiles)
{
    QMutexLocker locker(&mutex);
    for (auto desktop : desktopFiles) {
        for (auto sri = subRecoders.begin(); sri != subRecoders.end(); sri++) {
            if (!desktop.contains(sri.key()))
                continue;

            QFileInfo info(desktop);
            sri.value().uninstallMap[info.baseName()] = true;
        }
    }
}

/**
 * @brief AlRecorder::watchDirs 监控目录
 * @param dataDirs
 */
void AlRecorder::watchDirs(const QStringList &dataDirs)
{
    for (auto dirPath : dataDirs) {
        if (subRecoders.contains(dirPath))
            continue;

        QDir dir(dirPath);
        QStringList files = dir.entryList(QDir::Files);
        for (auto &file : files)
            file = dirPath + file;

        // 监听目录和文件
        watcher->addDir(dirPath);
        if (files.size() > 0)
            watcher->addPaths(files);

        // 初始化对应目录和应用信息
        initSubRecoder(dirPath);
    }
}

/**
 * @brief AlRecorder::initSubRecoder 初始化应用目录记录
 * @param dirPath
 */
void AlRecorder::initSubRecoder(const QString &dirPath)
{
    subRecorder sub;
    QByteArray encryText = QCryptographicHash::hash(dirPath.toLatin1(), QCryptographicHash::Md5);
    QString statusFile = userAppsCfgDir + "launched-" + encryText.toHex() + ".csv";

    // 读取App状态记录
    QMap<QString, bool> launchedApp;
    QFile file(statusFile);
    if (file.exists() && file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        while (!file.atEnd()){
            QString line(file.readLine());
            QStringList strs = line.split(",");
            if (strs.length() != 2)
                continue;

            if (strs[1].size() > 0 && strs[1][0] == 't')
                launchedApp[strs[0]] = true;
            else
                launchedApp[strs[0]] = false;
        }

        file.close();
    } else {
        // 读取app desktop
        QDir dir(dirPath);
        QStringList files = dir.entryList(QDir::Files);
        QStringList apps;
        for (QString file : files) {
            if (!file.endsWith(".desktop"))
                continue;

            int index = file.lastIndexOf(".");
            file.truncate(index);
            qInfo() << "entry =" << file;
            apps.push_back(file);
        }

        // fallback 都打开过
        for (auto app : apps)
            launchedApp[app] = true;
    }

    sub.statusFile = statusFile;
    sub.launchedMap = launchedApp;
    subRecoders[dirPath] = sub;
}

/**
 * @brief AlRecorder::onDFChanged 处理desktopFile文件改变事件
 * @param filePath
 * @param op
 */
void AlRecorder::onDFChanged(const QString &filePath, uint32_t op)
{
    QFileInfo info(filePath);
    QString dirPath = info.absolutePath() + "/";
    QString name = info.baseName();
    subRecorder &sub = subRecoders[dirPath];
    QMap<QString, bool> &launchedMap = sub.launchedMap;

    // 过滤文件， 仅保留.desktop类型
    if (!filePath.endsWith(".desktop"))
        return;

    switch (op) {
    case DFWatcher::event::Add:
        qInfo() << "AlRecorder: Add " << filePath;
        if (!launchedMap.contains(name)) {
            bool launched = false;
            if (sub.removedLaunchedMap.find(name) != sub.removedLaunchedMap.end()) {
                launched = sub.removedLaunchedMap[name];
                sub.removedLaunchedMap.remove(name);
            }
            launchedMap[name] = launched;
        }
        sub.uninstallMap.remove(name);
        saveStatusFile(dirPath); // 刷新状态文件
        break;
    case DFWatcher::event::Del:
        qInfo() << "AlRecorder: Del" << filePath;
        if (launchedMap.contains(name)) {
            if (!sub.uninstallMap.contains(name))
                sub.removedLaunchedMap[name] = launchedMap[name];

            launchedMap.remove(name);
        }
        saveStatusFile(dirPath); //刷新状态文件
        break;
    case DFWatcher::event::Mod:
        break;
    }
}

/**
 * @brief AlRecorder::saveStatusFile 保存状态文件
 * @param dirPath
 */
void AlRecorder::saveStatusFile(const QString &dirPath)
{
    subRecorder sub = subRecoders[dirPath];
    QString tmpFile = sub.statusFile + "_tmp";
    QFile fp(tmpFile);
    bool ok = false;
    qInfo() << "saveStatusFile=" << dirPath << "create file=" << tmpFile;
    if (fp.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream out(&fp);
        out << "# " << dirPath << endl;
        for (auto rl = sub.launchedMap.begin(); rl != sub.launchedMap.end(); rl++) {
            out << rl.key() << "," << ((rl.value() == true) ? "t" : "f") << endl;
        }
        ok = true;
        fp.close();
    }

    // 覆盖原文件
    QFile::remove(sub.statusFile);
    QFile::rename(tmpFile, sub.statusFile);
    Q_EMIT statusSaved(dirPath, sub.statusFile, ok);
}
