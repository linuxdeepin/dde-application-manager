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

#ifndef LAUNCHERSETTINGS_H
#define LAUNCHERSETTINGS_H

#include "common.h"

#include <QObject>
#include <QVector>

class LauncherSettings : public QObject
{
    Q_OBJECT
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

private:
    LauncherSettings(QObject *paret = nullptr);
    LauncherSettings(const LauncherSettings &);
    LauncherSettings& operator= (const LauncherSettings &);
};

#endif // LAUNCHERSETTINGS_H
