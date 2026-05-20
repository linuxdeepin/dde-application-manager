// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace EventReporter {
void reportAppLaunch(const QString &appName, qint64 timeMs, bool isLinglong, const QString &launchType = {}, const QString &uniqueID = {});
void reportAppLaunchFailed(const QString &appName, const QString &errors, bool isLinglong, const QString &launchType = {}, const QString &uniqueID = {});
void reportAppAbnormalExit(const QString &appName, const QString &launchType, const QString &exec, const QString &logInfo, bool isLinglong, const QString &uniqueID = {});
}
