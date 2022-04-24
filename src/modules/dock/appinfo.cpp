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

#include "appinfo.h"
#include "common.h"

#include <QDebug>
#include <QString>
#include <QCryptographicHash>

AppInfo::AppInfo(DesktopInfo &info)
 : isValid(true)
{
    init(info);
}

AppInfo::AppInfo(const QString &_fileName)
 : isValid(true)
{
    DesktopInfo info(_fileName.toStdString());
    init(info);
}

void AppInfo::init(DesktopInfo &info)
{
    if (!info.isValidDesktop()) {
        isValid = false;
        return;
    }

    std::string xDeepinVendor= info.kf.getStr(MainSection, "X-Deepin-Vendor");
    if (xDeepinVendor == "deepin") {
        name = info.getGenericName().c_str();
        if (name.isEmpty())
            name = info.getName().c_str();
    } else {
        name = info.getName().c_str();
    }

    innerId = genInnerIdWithDesktopInfo(info);
    fileName = info.getFileName().c_str();
    id = info.getId().c_str();
    icon = info.getIcon().c_str();
    installed = info.isInstalled();
    for (const auto & action : info.getActions()) {
        actions.push_back(action);
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
