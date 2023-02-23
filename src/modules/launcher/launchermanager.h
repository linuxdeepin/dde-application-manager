// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAUNCHERMANAGER_H
#define LAUNCHERMANAGER_H

#include <QObject>

class Launcher;
class LauncherManager : public QObject
{
    Q_OBJECT
public:
    explicit LauncherManager(QObject *parent = nullptr);
    ~LauncherManager();

private:
    Launcher *launcher;
};

#endif // LAUNCHERMANAGER_H
