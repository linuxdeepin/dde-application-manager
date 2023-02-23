// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYNCONFIG_H
#define SYNCONFIG_H

#include "synmodulebase.h"

#include <QObject>
#include <QMap>

class SynConfig final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Sync1.Config")
public:
    // 实例
    static SynConfig *instance(QObject *parent = nullptr);
    // 记录模块对应类信息
    static bool registe(QString moduleName, SynModuleBase *module);

public Q_SLOTS:
    // 获取配置信息
    QByteArray GetSyncConfig(QString moduleName);
    // 设置配置信息
    void SetSyncConfig(QString moduleName, QByteArray ba);

private:
    explicit SynConfig(QObject *parent = nullptr);
    ~SynConfig();

    // 将本服务注册到com.deepin.sync.Daemon
    void registerOnSyncDaemon();

    static QMap<QString, SynModuleBase*> synModulesMap; // 记录模块对应类
};

#endif // SYNCONFIG_H
