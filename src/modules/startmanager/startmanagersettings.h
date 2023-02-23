// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    DConfig *m_launchConfig;
    DConfig *m_startConfig;
    DConfig *m_xsettingsConfig;
};

#endif // STARTMANAGERSETTINGS_H
