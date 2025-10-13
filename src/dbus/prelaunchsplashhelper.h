// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PRELAUNCHSPLASHHELPER_H
#define PRELAUNCHSPLASHHELPER_H

#include <QObject>
#include <QtWaylandClient/QWaylandClientExtension>
#include <QLoggingCategory>

#include "qwayland-treeland-prelaunch-splash-v1.h"

Q_DECLARE_LOGGING_CATEGORY(amPrelaunchSplash)
class PrelaunchSplashHelper
    : public QWaylandClientExtensionTemplate<PrelaunchSplashHelper>
    , public QtWayland::treeland_prelaunch_splash_manager_v1
{
    Q_OBJECT
public:
    explicit PrelaunchSplashHelper();

    void show(const QString &appId);
};

#endif // PRELAUNCHSPLASHHELPER_H
