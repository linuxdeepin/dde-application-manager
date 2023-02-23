// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QObject>

class DFWatcher;
class AlRecorder;
class AppManager: public QObject
{
    Q_OBJECT
public:
    explicit AppManager(QObject *parent = nullptr);
    ~AppManager();

private:
    DFWatcher *watcher;
    AlRecorder *recorder;
};

#endif // APPMANAGER_H
