// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "prelaunchsplashhelper.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(amPrelaunchSplash, "dde.am.prelaunch.splash")

PrelaunchSplashHelper::PrelaunchSplashHelper()
    : QWaylandClientExtensionTemplate<PrelaunchSplashHelper>(1)
{

}

void PrelaunchSplashHelper::show(const QString &appId)
{
    // Check if the manager is active (i.e., connected to the compositor interface).
    if (!isActive()) {
        qCWarning(amPrelaunchSplash) << "Prelaunch splash manager is not active. Cannot show splash for" << appId;
        return;
    }

    // Send the create_splash request to the compositor.
    create_splash(appId);
    qCInfo(amPrelaunchSplash) << "Sent create_splash for" << appId;
}
