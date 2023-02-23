// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFWATCHER_H
#define DFWATCHER_H

#include <QObject>
#include <QMap>
#include <QFileSystemWatcher>

class DFWatcher: public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.DFWatcher1")

public:
    enum event{
        Add,
        Del,
        Mod,
    };

    explicit DFWatcher(QObject *parent = nullptr);
    ~DFWatcher();

    void addDir(const QString & path);
    void addPaths(const QStringList &paths);
    QStringList files();
    void removePath(const QString &filepath);
Q_SIGNALS:
    void Event(const QString &filepath, int op);

private Q_SLOTS:
    void fileUpdated(const QString &filePath);
    void directoryUpdated(const QString &dirPath);

private:
    QFileSystemWatcher  *watcher;
    QMap<QString, QStringList> dirContentMap; // 监控的目录内容列表
};

#endif
