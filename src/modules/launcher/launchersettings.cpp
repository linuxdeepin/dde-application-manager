// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "launchersettings.h"
#include "settings.h"

#include <DConfig>

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

DCORE_USE_NAMESPACE

LauncherSettings::LauncherSettings(QObject *parent)
 : QObject(parent)
 , m_dconfig(Settings::ConfigPtr(configLauncher))
{
    // 绑定属性
    connect(m_dconfig, &DConfig::valueChanged, this, [&] (const QString &key) {
        if (key == keyDisplayMode) {
            Q_EMIT displayModeChanged(m_dconfig->value(key).toString());
        } else if (key == keyFullscreen) {
            Q_EMIT fullscreenChanged(m_dconfig->value(key).toBool());
        } else if (key == keyAppsHidden) {
            Q_EMIT hiddenAppsChanged();
        }
    });
}

/**
 * @brief LauncherSettings::getDisplayMode 获取配置显示模式
 * @return
 */
QString LauncherSettings::getDisplayMode()
{
    return m_dconfig ? m_dconfig->value(keyDisplayMode).toString() : "";
}

/**
 * @brief LauncherSettings::setDisplayMode 设置配置显示模式
 * @param value
 */
void LauncherSettings::setDisplayMode(QString value)
{
    if (m_dconfig) {
        m_dconfig->setValue(keyDisplayMode, value);
    }
}

/**
 * @brief LauncherSettings::getFullscreenMode 获取配置全屏模式
 * @return
 */
int LauncherSettings::getFullscreenMode()
{
    return m_dconfig ? m_dconfig->value(keyFullscreen).toBool() : false;
}

/**
 * @brief LauncherSettings::setFullscreenMode 设置配置全屏模式
 * @param value 全屏模式
 */
void LauncherSettings::setFullscreenMode(int value)
{
    if (m_dconfig) {
        m_dconfig->setValue(keyFullscreen, value);
    }
}

/**
 * @brief LauncherSettings::getDisableScalingApps 获取配置禁用缩放应用
 * @return
 */
QVector<QString> LauncherSettings::getDisableScalingApps()
{
    QVector<QString> ret;
    if (m_dconfig) {
        QList<QVariant> apps = m_dconfig->value(keyAppsDisableScaling).toList();
        for (auto app : apps) {
            ret.push_back(app.toString());
        }
    }
    return ret;
}

/**
 * @brief LauncherSettings::setDisableScalingApps 设置配置禁用缩放应用
 * @param value 应用禁用缩放应用
 */
void LauncherSettings::setDisableScalingApps(const QVector<QString> &value)
{
    if (m_dconfig) {
        QList<QVariant> apps;
        for (const auto &app : value)
            apps.push_back(app);

        m_dconfig->setValue(keyAppsDisableScaling, apps);
    }
}

/**
 * @brief LauncherSettings::getUseProxyApps 获取配置代理应用
 * @return
 */
QVector<QString> LauncherSettings::getUseProxyApps()
{
    QVector<QString> ret;
    if (m_dconfig) {
        QList<QVariant> apps = m_dconfig->value(keyAppsUseProxy).toList();
        for (auto app : apps) {
            ret.push_back(app.toString());
        }
    }
    return ret;
}

/**
 * @brief LauncherSettings::setUseProxy 设置配置代理应用
 * @param value 代理应用
 */
void LauncherSettings::setUseProxy(const QVector<QString> &value)
{
    if (m_dconfig) {
        QList<QVariant> apps;
        for (const auto &app : value)
            apps.push_back(app);

        m_dconfig->setValue(keyAppsUseProxy, apps);
    }
}

/**
 * @brief LauncherSettings::getHiddenApps 获取配置隐藏应用
 * @return
 */
QVector<QString> LauncherSettings::getHiddenApps()
{
    QVector<QString> ret;
    if (m_dconfig) {
        QList<QVariant> hiddenApps = m_dconfig->value(keyAppsHidden).toList();
        for (auto app : hiddenApps) {
            ret.push_back(app.toString());
        }
    }
    return ret;
}
