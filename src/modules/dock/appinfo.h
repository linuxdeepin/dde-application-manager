// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
