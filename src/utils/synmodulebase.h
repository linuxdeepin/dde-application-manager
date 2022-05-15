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
    virtual void setSynConfig(QByteArray ba) = 0;
};



#endif // SYNCONFIGBASE_H
