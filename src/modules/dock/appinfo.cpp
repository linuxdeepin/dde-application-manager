// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appinfo.h"
#include "common.h"

#include <QDebug>
#include <QString>
#include <QCryptographicHash>

AppInfo::AppInfo(DesktopInfo &info)
 : m_isValid(true)
{
    init(info);
}

AppInfo::AppInfo(const QString &_fileName)
 : m_isValid(true)
{
    DesktopInfo info(_fileName.toStdString());
    init(info);
}

void AppInfo::init(DesktopInfo &info)
{
    if (!info.isValidDesktop()) {
        m_isValid = false;
        return;
    }

    std::string xDeepinVendor= info.getKeyFile()->getStr(MainSection, "X-Deepin-Vendor");
    if (xDeepinVendor == "deepin") {
        m_name = info.getGenericName().c_str();
        if (m_name.isEmpty())
            m_name = info.getName().c_str();
    } else {
        m_name = info.getName().c_str();
    }

    m_innerId = genInnerIdWithDesktopInfo(info);
    m_fileName = info.getFileName().c_str();
    m_id = info.getId().c_str();
    m_icon = info.getIcon().c_str();
    m_installed = info.isInstalled();
    for (const auto & action : info.getActions()) {
        m_actions.push_back(action);
    }
}

QString AppInfo::genInnerIdWithDesktopInfo(DesktopInfo &info)
{
    std::string cmdline = info.getCommandLine();
    QByteArray encryText = QCryptographicHash::hash(QString(cmdline.c_str()).toLatin1(), QCryptographicHash::Md5);
    QString innerId = desktopHashPrefix + encryText.toHex();
    qInfo() << "app " << info.getId().c_str() << " generate innerId :" << innerId;
    return innerId;
}
