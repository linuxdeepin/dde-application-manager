// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
