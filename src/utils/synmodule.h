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
    virtual void setSynConfig(QByteArray ba) = 0;
    // 注册配置模块
    virtual bool registeModule(QString moduleName) final {return SynConfig::instance(this)->registe(moduleName, this);}
};

#endif // SYNMODULE_H
