// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOCKSETTINGS_H
#define DOCKSETTINGS_H

#include "common.h"

#include <QObject>

// 隐藏模式
enum class HideMode {
    KeepShowing,
    KeepHidden,
    SmartHide
};

class HideModeHandler {
    HideMode modeEnum;
    QString modeStr;

public:
    HideModeHandler(HideMode mode) : modeEnum(mode), modeStr("") {}
    HideModeHandler(QString mode) : modeEnum(HideMode::KeepShowing), modeStr(mode) {}

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
        }
        // 默认保持始终显示
        return "keep-showing";
    }

    HideMode toEnum() {
        if (modeStr == "keep-hidden")
            return HideMode::KeepHidden;
        if (modeStr == "smart-hide")
            return HideMode::SmartHide;

        return HideMode::KeepShowing;
    }
};

// 显示样式
enum class DisplayMode {
    Fashion,
    Efficient
};

class DisplayModeHandler {
    DisplayMode modeEnum;
    QString modeStr;

public:
    DisplayModeHandler(DisplayMode mode) : modeEnum(mode), modeStr("") {}
    DisplayModeHandler(QString mode) : modeEnum(DisplayMode::Efficient), modeStr(mode) {}

    bool equal(DisplayModeHandler displayMode) {
        return toString() == displayMode.toString() || toEnum() == displayMode.toEnum();
    }

    QString toString() {
        switch (modeEnum) {
        case DisplayMode::Fashion:
            return "fashion";
        case DisplayMode::Efficient:
            return "efficient";
        }
    }

    DisplayMode toEnum() {
        if (modeStr == "fashion")
            return DisplayMode::Fashion;

        return DisplayMode::Efficient;
    }
};

// 显示位置
enum class PositionMode {
    Top,    // 上
    Right,  // 右
    Bottom, // 下
    Left    // 左
};

class PositionModeHandler {
    PositionMode modeEnum;
    QString modeStr;

public:
    PositionModeHandler(PositionMode mode) : modeEnum(mode), modeStr("") {}
    PositionModeHandler(QString mode) : modeEnum(PositionMode::Bottom), modeStr(mode) {}

    bool equal(PositionModeHandler displayMode) {
        return toString() == displayMode.toString() || toEnum() == displayMode.toEnum();
    }

    QString toString() {
        switch (modeEnum) {
        case PositionMode::Top:
            return "top";
        case PositionMode::Right:
            return "right";
        case PositionMode::Left:
            return "left";
        case PositionMode::Bottom:
            return "bottom";
        }
    }

    PositionMode toEnum() {
        if (modeStr == "top")
            return PositionMode::Top;
        if (modeStr == "right")
            return PositionMode::Right;
        if (modeStr == "bottom")
            return PositionMode::Bottom;
        if (modeStr == "left")
            return PositionMode::Left;

        return PositionMode::Bottom;
    }
};

// 强制退出应用菜单状态
enum class ForceQuitAppMode {
    Enabled,        // 开启
    Disabled,       // 关闭
    Deactivated,    // 置灰
};

class ForceQuitAppModeHandler {
    ForceQuitAppMode modeEnum;
    QString modeStr;

public:
    ForceQuitAppModeHandler(ForceQuitAppMode mode) : modeEnum(mode), modeStr("") {}
    ForceQuitAppModeHandler(QString mode) : modeEnum(ForceQuitAppMode::Enabled), modeStr(mode) {}

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
        }
    }

    ForceQuitAppMode toEnum() {
        if (modeStr == "disabled")
            return ForceQuitAppMode::Disabled;
        if (modeStr == "deactivated")
            return ForceQuitAppMode::Deactivated;

        return ForceQuitAppMode::Enabled;
    }
};

class Settings;
namespace Dtk {
namespace Core {
class DConfig;
}
}

using namespace Dtk::Core;

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
    void setDockedApps(const QStringList &apps);
    QStringList getRecentApps() const;
    void setRecentApps(const QStringList &apps);
    double getOpacity();
    QVector<QString> getWinIconPreferredApps();
    void setShowRecent(bool visible);
    bool showRecent() const;

    void setShowMultiWindow(bool showMultiWindow);
    bool showMultiWindow() const;

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
    // 是否显示最近打开应用改变
    void showRecentChanged(bool);
    // 是否显示多开应用改变
    void showMultiWindowChanged(bool);

private:
    DockSettings(QObject *paret = nullptr);
    DockSettings(const DockSettings &);
    DockSettings& operator= (const DockSettings &);
    void saveStringList(const QString &key, const QStringList &values);
    QStringList loadStringList(const QString &key) const;

private:
    DConfig *m_dockSettings;
    DConfig *m_appearanceSettings;
};

#endif // DOCKSETTINGS_H
