// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYNMODULE_H
#define SYNMODULE_H

#include "synconfig.h"
#include <QString>

class SynModule : public SynModuleBase, public QObject
{
public:
    explicit SynModule(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~SynModule() {}

    // 获取配置信息
    virtual QByteArray getSyncConfig() = 0;
    // 设置配置信息
    virtual void setSyncConfig(QByteArray ba) = 0;
    // 注册配置模块
    virtual bool registeModule(QString moduleName) final {return SynConfig::instance(this)->registe(moduleName, this);}
};

#endif // SYNMODULE_H
