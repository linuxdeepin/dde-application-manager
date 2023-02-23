// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef STARTMANAGERDBUSHANDLER_H
#define STARTMANAGERDBUSHANDLER_H

#include <QObject>

class StartManagerDBusHandler : public QObject
{
    Q_OBJECT
public:
    explicit StartManagerDBusHandler(QObject *parent = nullptr);

    void markLaunched(QString desktopFile);

    QString getProxyMsg();
    void addProxyProc(int32_t pid);

Q_SIGNALS:

public Q_SLOTS:
};

#endif // STARTMANAGERDBUSHANDLER_H
