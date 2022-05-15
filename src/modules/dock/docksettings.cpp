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

#include "docksettings.h"
#include "settings.h"

#include <DConfig>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

DCORE_USE_NAMESPACE

static DConfig *dockSettings = Settings::ConfigPtr(configDock);
static DConfig *appearanceSettings = Settings::ConfigPtr(configAppearance);

DockSettings::DockSettings(QObject *parent)
 : QObject (parent)
{
    init();
}

void DockSettings::init()
{
    // 绑定属性
    if (dockSettings) {
            connect(dockSettings, &DConfig::valueChanged, this, [&] (const QString &key) {
                if (key == keyHideMode) {
                    Q_EMIT hideModeChanged(HideMode(dockSettings->value(key).toInt()));
                } else if(key == keyDisplayMode) {
                    Q_EMIT displayModeChanged(DisplayMode(dockSettings->value(key).toInt()));
                } else if (key == keyPosition) {
                    Q_EMIT positionModeChanged(PositonMode(dockSettings->value(key).toInt()));
                } else if (key == keyForceQuitApp){
                    Q_EMIT forceQuitAppChanged(dockSettings->value(key).toBool());
                }
            });
    }
}

HideMode DockSettings::getHideMode()
{
    HideMode ret = HideMode::Unknown;
    if (dockSettings) {
        QString mode = dockSettings->value(keyHideMode).toString();
        HideModeHandler handler(mode);
        ret = handler.toEnum();
    }
    return ret;
}

void DockSettings::setHideMode(HideMode mode)
{
    if (dockSettings) {
        dockSettings->setValue(keyHideMode, HideModeHandler(mode).toString());
    }
}

DisplayMode DockSettings::getDisplayMode()
{
    DisplayMode ret = DisplayMode::Unknown;
    if (dockSettings) {
        QString mode = dockSettings->value(keyDisplayMode).toString();
        DisplayModeHandler handler(mode);
        ret = handler.toEnum();
    }
    return ret;
}

void DockSettings::setDisplayMode(DisplayMode mode)
{
    if (dockSettings) {
        dockSettings->setValue(keyHideMode, DisplayModeHandler(mode).toString());
    }
}

PositonMode DockSettings::getPositionMode()
{
    PositonMode ret = PositonMode::Unknown;
    if (dockSettings) {
        QString mode = dockSettings->value(keyPosition).toString();
        PositonModeHandler handler(mode);
        ret = handler.toEnum();
    }
    return ret;
}

void DockSettings::setPositionMode(PositonMode mode)
{
    if (dockSettings) {
        dockSettings->setValue(keyPosition, PositonModeHandler(mode).toString());
    }
}

ForceQuitAppMode DockSettings::getForceQuitAppMode()
{
    ForceQuitAppMode ret = ForceQuitAppMode::Unknown;
    if (dockSettings) {
        QString mode = dockSettings->value(keyForceQuitApp).toString();
        ForceQuitAppModeHandler handler(mode);
        ret = handler.toEnum();
    }
    return ret;
}

void DockSettings::setForceQuitAppMode(ForceQuitAppMode mode)
{
    if (dockSettings) {
        dockSettings->setValue(keyForceQuitApp, ForceQuitAppModeHandler(mode).toString());
    }
}

uint DockSettings::getIconSize()
{
    uint size = 36;
    if (dockSettings) {
        size = dockSettings->value(keyIconSize).toUInt();
    }
    return size;
}

void DockSettings::setIconSize(uint size)
{
    if (dockSettings) {
        dockSettings->setValue(keyIconSize, size);
    }
}

uint DockSettings::getShowTimeout()
{
    uint time = 100;
    if (dockSettings) {
        time = dockSettings->value(keyShowTimeout).toUInt();
    }
    return time;
}

void DockSettings::setShowTimeout(uint time)
{
    if (dockSettings) {
        dockSettings->setValue(keyShowTimeout, time);
    }
}

uint DockSettings::getHideTimeout()
{
    uint time = 0;
    if (dockSettings) {
        time = dockSettings->value(keyHideTimeout).toUInt();
    }
    return time;
}

void DockSettings::setHideTimeout(uint time)
{
    if (dockSettings) {
        dockSettings->setValue(keyHideTimeout, time);
    }
}

uint DockSettings::getWindowSizeEfficient()
{
    uint size = 40;
    if (dockSettings) {
        size = dockSettings->value(keyWindowSizeEfficient).toUInt();
    }
    return size;
}

void DockSettings::setWindowSizeEfficient(uint size)
{
    if (dockSettings) {
        dockSettings->setValue(keyWindowSizeEfficient, size);
    }
}

uint DockSettings::getWindowSizeFashion()
{
    uint size = 48;
    if (dockSettings) {
        size = dockSettings->value(keyWindowSizeFashion).toUInt();
    }
    return size;
}

void DockSettings::setWindowSizeFashion(uint size)
{
    if (dockSettings) {
        dockSettings->setValue(keyWindowSizeFashion, size);
    }
}

QStringList DockSettings::getDockedApps()
{
    QStringList ret;
    if (!dockSettings)
        return ret;

    for(const auto &var : dockSettings->value(keyDockedApps).toList()) {
        if (var.isValid())
            ret.push_back(var.toString());
    }

    return ret;
}

void DockSettings::setDockedApps(QList<QString> &apps)
{
    if (!dockSettings)
        return;

    qDebug() << "docked apps:" << apps;
    QVariantList list;
    for (auto app : apps) {
        list << QVariant(app);
    }

    dockSettings->setValue(keyDockedApps, list);
}

double DockSettings::getOpacity()
{
    double opacity = 0.4;
    if (appearanceSettings) {
        opacity = appearanceSettings->value(keyOpacity).toDouble();
    }
    return opacity;
}

QVector<QString> DockSettings::getWinIconPreferredApps()
{
    QVector<QString> ret;
    if (dockSettings) {
        for(const auto &var : dockSettings->value(keyWinIconPreferredApps).toList()) {
            if (var.isValid())
                ret.push_back(var.toString());
        }
    }

    return ret;
}

QString DockSettings::getPluginSettings()
{
    QString ret;
    if (dockSettings) {
        ret = dockSettings->value(keyPluginSettings).toString();
    }

    qInfo() << "getpluginsettings:" << ret;
    return ret;
}

void DockSettings::setPluginSettings(QString jsonStr)
{
    if (jsonStr.isEmpty())
        return;

    if (dockSettings) {
        dockSettings->setValue(keyPluginSettings, jsonStr);
    }
}

void DockSettings::mergePluginSettings(QString jsonStr)
{
    QString origin = getPluginSettings();
    QJsonObject originSettings = plguinSettingsStrToObj(origin);
    QJsonObject needMergeSettings = plguinSettingsStrToObj(jsonStr);
    for (auto pluginsIt = needMergeSettings.begin(); pluginsIt != needMergeSettings.end(); pluginsIt++) {
        const QString &pluginName = pluginsIt.key();
        const QJsonObject &needMergeSettingsObj = pluginsIt.value().toObject();
        QJsonObject originSettingsObj = originSettings.value(pluginName).toObject();
        for (auto settingsIt = needMergeSettingsObj.begin(); settingsIt != needMergeSettingsObj.end(); settingsIt++) {
            originSettingsObj.insert(settingsIt.key(), settingsIt.value());
        }

        // 重写plugin对应的设置
        originSettings.remove(pluginName);
        originSettings.insert(pluginName, originSettingsObj);
    }

    setPluginSettings(QJsonDocument(originSettings).toJson(QJsonDocument::JsonFormat::Compact));
}

void DockSettings::removePluginSettings(QString pluginName, QStringList settingkeys)
{
    if (pluginName.isEmpty())
        return;

    QString origin = getPluginSettings();
    QJsonObject originSettings = plguinSettingsStrToObj(origin);
    if (settingkeys.size() == 0) {
        originSettings.remove(pluginName);
    } else {
        for (auto pluginsIt = originSettings.begin(); pluginsIt != originSettings.end(); pluginsIt++) {
            const QString &pluginName = pluginsIt.key();
            if (pluginName != pluginName)
                continue;

            QJsonObject originSettingsObj = originSettings.value(pluginName).toObject();
            for (const auto &key : settingkeys) {
                originSettingsObj.remove(key);
            }

            // 重写plugin对应的设置
            originSettings.remove(pluginName);
            originSettings.insert(pluginName, originSettingsObj);
        }
    }

    setPluginSettings(QJsonDocument(originSettings).toJson(QJsonDocument::JsonFormat::Compact));
}

// 借鉴自dde-dock
QJsonObject DockSettings::plguinSettingsStrToObj(QString jsonStr)
{
    QJsonObject ret;
    const QJsonObject &pluginSettingsObject = QJsonDocument::fromJson(jsonStr.toLocal8Bit()).object();
    if (pluginSettingsObject.isEmpty()) {
        return ret;
    }

    for (auto pluginsIt = pluginSettingsObject.constBegin(); pluginsIt != pluginSettingsObject.constEnd(); ++pluginsIt) {
        const QString &pluginName = pluginsIt.key();
        const QJsonObject &settingsObject = pluginsIt.value().toObject();
        QJsonObject newSettingsObject = ret.value(pluginName).toObject();
        for (auto settingsIt = settingsObject.constBegin(); settingsIt != settingsObject.constEnd(); ++settingsIt) {
            newSettingsObject.insert(settingsIt.key(), settingsIt.value());
        }
        // TODO: remove not exists key-values
        ret.insert(pluginName, newSettingsObject);
    }

    return ret;
}

