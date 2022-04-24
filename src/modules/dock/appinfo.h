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

#ifndef APPINFO_H
#define APPINFO_H

#include "desktopinfo.h"

#include<QVector>

// 应用信息类
class AppInfo
{
public:
    explicit AppInfo(DesktopInfo &info);
    explicit AppInfo(const QString &_fileName);

    void init(DesktopInfo &info);
    QString getFileName() {return fileName;}
    QString getIcon() {return icon;}
    QString getId() {return id;}
    QString getInnerId() {return innerId;}
    QString getName() {return name;}
    QVector<DesktopAction> getActions() {return actions;}
    QString getIdentifyMethod() {return identifyMethod;}
    void setIdentifyMethod(QString method) {identifyMethod = method;}
    bool isInstalled() {return installed;}
    bool isValidApp() {return isValid;}

private:
    QString genInnerIdWithDesktopInfo(DesktopInfo &info);

    QString fileName;
    QString id;
    QString icon;
    QString identifyMethod;
    QString innerId;
    QString name;
    QVector<DesktopAction> actions;
    bool installed;
    bool isValid;
};

#endif // APPINFO_H
