// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ALRECORDER_H
#define ALRECORDER_H

#include <QObject>
#include <QMap>
#include <QMutex>

class DFWatcher;

// 记录当前用户应用状态信息
class AlRecorder: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.AlRecorder1")

public:
    // 各个应用目录中应用的启动记录
    struct subRecorder {
        QString statusFile;                     // 应用目录状态文件
        QMap<QString, bool> launchedMap;        // 应用启动记录
        QMap<QString, bool> removedLaunchedMap; // desktop文件卸载记录
        QMap<QString, bool> uninstallMap;       // 记录应用将被卸载状态
    };

    AlRecorder(DFWatcher *_watcher, QObject *parent = nullptr);
    ~AlRecorder();

Q_SIGNALS:
    void launched(const QString &file);
    void statusSaved(const QString &root, const QString &file, bool ok);
    void serviceRestarted();

private Q_SLOTS:
    void onDFChanged(const QString &filePath, uint32_t op);

public Q_SLOTS:
    QMap<QString, QStringList> getNew();
    void markLaunched(const QString &filePath);
    void uninstallHints(const QStringList &desktopFiles);
    void watchDirs(const QStringList &dataDirs);

private:
    void initSubRecoder(const QString &dirPath);
    void saveStatusFile(const QString &dirPath);

    QMap<QString,subRecorder> subRecoders;  // 记录不同应用目录的文件状态
    DFWatcher *watcher;
    QMutex mutex;
};

#endif // ALRECODER_H
