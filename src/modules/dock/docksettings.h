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

#ifndef DOCKSETTINGS_H
#define DOCKSETTINGS_H

#include "common.h"

#include <QObject>

// 隐藏模式
enum class HideMode {
    KeepShowing,
    KeepHidden,
    SmartHide,
    Unknown,
};

class HideModeHandler {
    HideMode modeEnum;
    QString modeStr;

public:
    HideModeHandler(HideMode mode) : modeEnum(mode), modeStr("") {}
    HideModeHandler(QString mode) : modeEnum(HideMode::Unknown), modeStr(mode) {}

    bool equal(HideModeHandler hideMode) {
        return toString() == hideMode.toString() || toEnum() == hideMode.toEnum();
    }

    QString toString() {
        switch (modeEnum) {
        case HideMode::KeepShowing:
            return "keep-showing";
        case HideMode::KeepHidden:
            return "keep-hidden";
        case HideMode::SmartHide:
            return "smart-hide";
        case HideMode::Unknown:
        default:
            return "unknown";
        }
    }

    HideMode toEnum() {
        if (modeStr == "keep-showing")
            return HideMode::KeepHidden;
        else if (modeStr == "keep-hidden")
            return HideMode::KeepHidden;
        else if (modeStr == "smart-hide")
            return HideMode::SmartHide;
        else
            return HideMode::Unknown;
    }
};

// 显示样式
enum class DisplayMode {
    Fashion,
    Efficient,
    Unknown,
};

class DisplayModeHandler {
    DisplayMode modeEnum;
    QString modeStr;

public:
    DisplayModeHandler(DisplayMode mode) : modeEnum(mode), modeStr("") {}
    DisplayModeHandler(QString mode) : modeEnum(DisplayMode::Unknown), modeStr(mode) {}

    bool equal(DisplayModeHandler displayMode) {
        return toString() == displayMode.toString() || toEnum() == displayMode.toEnum();
    }

    QString toString() {
        switch (modeEnum) {
        case DisplayMode::Fashion:
            return "fashion";
        case DisplayMode::Efficient:
            return "efficient";
        case DisplayMode::Unknown:
        default:
            return "unknown";
        }
    }

    DisplayMode toEnum() {
        if (modeStr == "fashion")
            return DisplayMode::Fashion;
        else if (modeStr == "efficient")
            return DisplayMode::Efficient;
        else
            return DisplayMode::Unknown;
    }
};

// 显示位置
enum class PositionMode {
    TOP,
    Right,
    Bottom,
    Left,
    Unknown,
};

class PositionModeHandler {
    PositionMode modeEnum;
    QString modeStr;

public:
    PositionModeHandler(PositionMode mode) : modeEnum(mode), modeStr("") {}
    PositionModeHandler(QString mode) : modeEnum(PositionMode::Unknown), modeStr(mode) {}

    bool equal(PositionModeHandler displayMode) {
        return toString() == displayMode.toString() || toEnum() == displayMode.toEnum();
    }

    QString toString() {
        switch (modeEnum) {
        case PositionMode::TOP:
            return "top";
        case PositionMode::Right:
            return "right";
        case PositionMode::Bottom:
            return "bottom";
        case PositionMode::Left:
            return "left";
        case PositionMode::Unknown:
        default:
            return "unknown";
        }
    }

    PositionMode toEnum() {
        if (modeStr == "top")
            return PositionMode::TOP;
        else if (modeStr == "right")
            return PositionMode::Right;
        else if (modeStr == "bottom")
            return PositionMode::Bottom;
        else if (modeStr == "left")
            return PositionMode::Left;
        else
            return PositionMode::Unknown;
    }
};

// 强制退出应用菜单状态
enum class ForceQuitAppMode {
    Enabled,        // 开启
    Disabled,       // 关闭
    Deactivated,    // 置灰
    Unknown
};

class ForceQuitAppModeHandler {
    ForceQuitAppMode modeEnum;
    QString modeStr;

public:
    ForceQuitAppModeHandler(ForceQuitAppMode mode) : modeEnum(mode), modeStr("") {}
    ForceQuitAppModeHandler(QString mode) : modeEnum(ForceQuitAppMode::Unknown), modeStr(mode) {}

    bool equal(ForceQuitAppModeHandler displayMode) {
        return toString() == displayMode.toString() || toEnum() == displayMode.toEnum();
    }

    QString toString() {
        switch (modeEnum) {
        case ForceQuitAppMode::Enabled:
            return "enabled";
        case ForceQuitAppMode::Disabled:
            return "disabled";
        case ForceQuitAppMode::Deactivated:
            return "deactivated";
        case ForceQuitAppMode::Unknown:
        default:
            return "unknown";
        }
    }

    ForceQuitAppMode toEnum() {
        if (modeStr == "enabled")
            return ForceQuitAppMode::Enabled;
        else if (modeStr == "disabled")
            return ForceQuitAppMode::Disabled;
        else if (modeStr == "deactivated")
            return ForceQuitAppMode::Deactivated;
        else
            return ForceQuitAppMode::Unknown;
    }
};

class Settings;

// 任务栏组策略配置类
class DockSettings: public QObject
{
    Q_OBJECT

public:
    static inline DockSettings *instance() {
        static DockSettings instance;
        return &instance;
    }
    void init();

    HideMode getHideMode();
    void setHideMode(HideMode mode);
    DisplayMode getDisplayMode();
    void setDisplayMode(DisplayMode mode);
    PositionMode getPositionMode();
    void setPositionMode(PositionMode mode);
    ForceQuitAppMode getForceQuitAppMode();
    void setForceQuitAppMode(ForceQuitAppMode mode);
    uint getIconSize();
    void setIconSize(uint size);
    uint getShowTimeout();
    void setShowTimeout(uint time);
    uint getHideTimeout();
    void setHideTimeout(uint time);
    uint getWindowSizeEfficient();
    void setWindowSizeEfficient(uint size);
    uint getWindowSizeFashion();
    void setWindowSizeFashion(uint size);
    QStringList getDockedApps();
    void setDockedApps(QList<QString> &apps);
    double getOpacity();
    QVector<QString> getWinIconPreferredApps();

    // plugin settings
    QString getPluginSettings();
    void setPluginSettings(QString jsonStr);
    void mergePluginSettings(QString jsonStr);
    void removePluginSettings(QString pluginName, QStringList settingkeys);
    QJsonObject plguinSettingsStrToObj(QString jsonStr);


Q_SIGNALS:
    // 隐藏模式改变
    void hideModeChanged(HideMode mode);
    // 显示样式改变
    void displayModeChanged(DisplayMode mode);
    // 显示位置改变
    void positionModeChanged(PositionMode mode);
    // 强制退出应用开关改变
    void forceQuitAppChanged(ForceQuitAppMode mode);


private:
    DockSettings(QObject *paret = nullptr);
    DockSettings(const DockSettings &);
    DockSettings& operator= (const DockSettings &);
};

#endif // DOCKSETTINGS_H
