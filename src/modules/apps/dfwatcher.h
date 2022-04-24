/*
 * Copyright (C) 2022 ~ 2023 Deepin Technology Co., Ltd.
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

#ifndef DFWATCHER_H
#define DFWATCHER_H

#include <QObject>
#include <QMap>
#include <QFileSystemWatcher>

class DFWatcher: public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.daemon.DFWatcher1")

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
