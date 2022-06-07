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

#ifndef STARTMANAGERSETTINGS_H
#define STARTMANAGERSETTINGS_H
#ifdef signals
#undef signals
#endif

#include "common.h"
#include "gsetting.h"

#include <QObject>
#include <QVector>

namespace Dtk {
namespace Core {
class DConfig;
}
}

using namespace Dtk::Core;

class StartManagerSettings : public QObject
{
    Q_OBJECT
public:
    static inline StartManagerSettings *instance() {
        static StartManagerSettings instance;
        return &instance;
    }

    QVector<QString> getUseProxyApps();
    QVector<QString> getDisableScalingApps();

    bool getMemCheckerEnabled();

    double getScaleFactor();

    QString getDefaultTerminalExec();
    QString getDefaultTerminalExecArg();

Q_SIGNALS:

private:
    StartManagerSettings(QObject *parent = nullptr);
    StartManagerSettings(const StartManagerSettings &);
    StartManagerSettings& operator= (const StartManagerSettings &);

    DConfig *launchConfig;
    DConfig *startConfig;
    DConfig *xsettingsConfig;
};

#endif // STARTMANAGERSETTINGS_H
