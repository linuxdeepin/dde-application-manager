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
    QString getFileName() {return m_fileName;}
    QString getIcon() {return m_icon;}
    QString getId() {return m_id;}
    QString getInnerId() {return m_innerId;}
    QString getName() {return m_name;}
    QVector<DesktopAction> getActions() {return m_actions;}
    QString getIdentifyMethod() {return m_identifyMethod;}
    void setIdentifyMethod(QString method) {m_identifyMethod = method;}
    bool isInstalled() {return m_installed;}
    bool isValidApp() {return m_isValid;}

private:
    QString genInnerIdWithDesktopInfo(DesktopInfo &info);

    QString m_fileName;
    QString m_id;
    QString m_icon;
    QString m_identifyMethod;
    QString m_innerId;
    QString m_name;
    QVector<DesktopAction> m_actions;
    bool m_installed;
    bool m_isValid;
};

#endif // APPINFO_H
