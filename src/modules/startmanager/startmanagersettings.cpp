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

#include "startmanagersettings.h"
#include "settings.h"
#include "gsetting.h"

#include <DConfig>

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

DCORE_USE_NAMESPACE

StartManagerSettings::StartManagerSettings(QObject *parent)
 : QObject (parent)
 , m_launchConfig(Settings::ConfigPtr(configLauncher))
 , m_startConfig(Settings::ConfigPtr(configStartdde))
 , m_xsettingsConfig(Settings::ConfigPtr(configXsettings))
{

}

QVector<QString> StartManagerSettings::getUseProxyApps()
{
    QVector<QString> ret;
    if (!m_launchConfig)
        return ret;

    QList<QVariant> apps = m_launchConfig->value(keyAppsUseProxy).toList();
    for (auto app : apps) {
        ret.push_back(app.toString());
    }

    return ret;
}

QVector<QString> StartManagerSettings::getDisableScalingApps()
{
    QVector<QString> ret;
    if (!m_launchConfig)
        return ret;

    QList<QVariant> apps = m_launchConfig->value(keyAppsDisableScaling).toList();
    for (auto app : apps) {
        ret.push_back(app.toString());
    }

    return ret;
}

bool StartManagerSettings::getMemCheckerEnabled()
{
    bool ret = false;
    if (m_startConfig) {
        ret = m_startConfig->value(keyMemCheckerEnabled).toBool();
    }
    return ret;
}

double StartManagerSettings::getScaleFactor()
{
    double ret = 0;
    if (m_xsettingsConfig) {
        m_xsettingsConfig->value(keyScaleFactor).toDouble();
    }
    return ret;
}

QString StartManagerSettings::getDefaultTerminalExec()
{
    QString ret;
    GSetting settings("com.deepin.desktop.default-applications.terminal");
    auto exec = settings.getString("exec");
    if (!exec.empty()) {
        ret = exec.c_str();
    }
    return ret;
}

QString StartManagerSettings::getDefaultTerminalExecArg()
{
    QString ret;
    GSetting settings("com.deepin.desktop.default-applications.terminal");
    auto execArg = settings.getString("exec-arg");
    if (!execArg.empty()) {
        ret = execArg.c_str();
    }
    return ret;
}
