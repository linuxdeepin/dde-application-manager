// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace EventReporter {
void reportAppLaunch(const QString &appName, const QString &appVersion, qint64 timeMs, const QString &launchType = {}, const QString &uniqueID = {});
void reportAppLaunchFailed(const QString &appName, const QString &appVersion, const QString &errors, const QString &launchType = {}, const QString &uniqueID = {});
void reportAppLaunchDuration(const QString &appName, const QString &appVersion, qint64 timeMs, const QString &launchType = {}, const QString &uniqueID = {});
void reportAppAbnormalExit(const QString &appName, const QString &launchType, const QString &exec, const QString &logInfo, const QString &uniqueID = {});

QString getAppVersion(const QString &appId);
}
