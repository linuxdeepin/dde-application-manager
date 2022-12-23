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
