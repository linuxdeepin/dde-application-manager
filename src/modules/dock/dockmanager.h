// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOCKMANAGER_H
#define DOCKMANAGER_H

#include "dbusadaptordock.h"

#include <QObject>

class Dock;
class ApplicationManager;

// 任务栏管理类
class DockManager : public QObject
{
    Q_OBJECT
public:
    explicit DockManager(ApplicationManager *parent = nullptr);
    ~DockManager();

private:
    Dock *m_dock;
    DBusAdaptorDock *m_adaptor;
};

#endif // DOCKMANAGER_H
