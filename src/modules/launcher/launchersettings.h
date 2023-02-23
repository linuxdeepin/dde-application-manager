// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAUNCHERSETTINGS_H
#define LAUNCHERSETTINGS_H

#include "common.h"

#include <QObject>
#include <QVector>

namespace Dtk {
namespace Core {
class DConfig;
}
}

using namespace Dtk::Core;

// 启动器相关配置
class LauncherSettings : public QObject
{
    Q_OBJECT
    LauncherSettings(QObject *paret = nullptr);
    LauncherSettings(const LauncherSettings &);
    LauncherSettings& operator= (const LauncherSettings &);
    DConfig *m_dconfig;

public:
    static inline LauncherSettings *instance() {
        static LauncherSettings instance;
        return &instance;
    }

    QString getDisplayMode();
    void setDisplayMode(QString value);

    int getFullscreenMode();
    void setFullscreenMode(int value);

    QVector<QString> getDisableScalingApps();
    void setDisableScalingApps(const QVector<QString> &value);

    QVector<QString> getUseProxyApps();
    void setUseProxy(const QVector<QString> &value);

    QVector<QString> getHiddenApps();

Q_SIGNALS:
    void displayModeChanged(QString mode);
    void fullscreenChanged(bool isFull);
    void hiddenAppsChanged();
};

#endif // LAUNCHERSETTINGS_H
