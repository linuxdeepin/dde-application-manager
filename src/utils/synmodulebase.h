// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYNMODULEBASE_H
#define SYNMODULEBASE_H

#include <QObject>
#include <QString>
#include <QByteArray>

// 同步配置基类
class SynModuleBase
{
public:
    SynModuleBase() {}
    virtual ~SynModuleBase() {}

    // 获取配置信息
    virtual QByteArray getSyncConfig() = 0;
    // 设置配置信息
    virtual void setSyncConfig(QByteArray ba) = 0;
};



#endif // SYNCONFIGBASE_H
