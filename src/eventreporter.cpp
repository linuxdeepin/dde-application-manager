// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "eventreporter.h"

#ifdef HAVE_DDE_API_EVENTLOGGER
#include <dde-api/eventlogger.hpp>
#endif
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QHash>

Q_LOGGING_CATEGORY(amEventReporter, "dde.am.event.reporter")

namespace {
QHash<QString, QString> &versionCache()
{
    static QHash<QString, QString> cache;
    return cache;
}

QHash<QString, QString> &pakTypeCache()
{
    static QHash<QString, QString> cache;
    return cache;
}

struct AppPackageInfo {
    QString version;
    QString pakType;
};

AppPackageInfo queryAppPackageInfo(const QString &appId, bool isLinglong)
{
    auto &vCache = versionCache();
    auto &pCache = pakTypeCache();

    bool versionCached = vCache.contains(appId);
    bool pakTypeCached = pCache.contains(appId);
    if (versionCached && pakTypeCached) {
        return {vCache.value(appId), pCache.value(appId)};
    }

    AppPackageInfo info{versionCached ? vCache.value(appId) : QString{},
                        pakTypeCached ? pCache.value(appId) : QString{}};

    qCDebug(amEventReporter) << "query package info for appId:" << appId;

    if (isLinglong) {
        QProcess proc;
        proc.start("ll-cli", {"info", appId});
        if (!proc.waitForStarted(1000)) {
            qCWarning(amEventReporter) << "ll-cli failed to start for" << appId;
        } else if (!proc.waitForFinished(1000)) {
            qCWarning(amEventReporter) << "ll-cli timeout for" << appId;
            proc.kill();
        } else if (proc.exitCode() == 0) {
            auto doc = QJsonDocument::fromJson(proc.readAllStandardOutput());
            if (doc.isObject()) {
                info.version = doc.object().value("version").toString();
            }
            info.pakType = "linglong";
        } else {
            qCWarning(amEventReporter) << "ll-cli query failed for" << appId << "exitCode:" << proc.exitCode();
        }
    } else {
        QProcess proc;
        proc.start("dpkg-query", {"-W", "-f=${Version}", appId});
        if (!proc.waitForStarted(1000)) {
            qCWarning(amEventReporter) << "dpkg-query failed to start for" << appId;
        } else if (!proc.waitForFinished(1000)) {
            qCWarning(amEventReporter) << "dpkg-query timeout for" << appId;
            proc.kill();
        } else if (proc.exitCode() == 0) {
            info.version = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            info.pakType = "deb";
        } else {
            qCWarning(amEventReporter) << "dpkg-query failed for" << appId << "exitCode:" << proc.exitCode();
        }
    }

    if (info.pakType.isEmpty()) {
        info.pakType = "unknown";
    }

    vCache.insert(appId, info.version);
    pCache.insert(appId, info.pakType);

    return info;
}
}

void EventReporter::reportAppLaunch(const QString &appName, qint64 timeMs, bool isLinglong, const QString &launchType, const QString &uniqueID)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    auto info = queryAppPackageInfo(appName, isLinglong);
    DDE_EventLogger::EventLogger::instance().writeEventLog({
        1000610001,
        appName,
        QJsonObject{
            {"app_name", appName},
            {"launch_type", launchType},
            {"app_version", info.version},
            {"unique_id", uniqueID},
            {"time", timeMs},
            {"app_package_type", info.pakType},
        },
    });
#else
    Q_UNUSED(appName)
    Q_UNUSED(timeMs)
    Q_UNUSED(launchType)
    Q_UNUSED(uniqueID)
#endif
}

void EventReporter::reportAppLaunchFailed(const QString &appName, const QString &errors, bool isLinglong, const QString &launchType, const QString &uniqueID)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    auto info = queryAppPackageInfo(appName, isLinglong);
    DDE_EventLogger::EventLogger::instance().writeEventLog({
        1000610002,
        appName,
        QJsonObject{
            {"app_name", appName},
            {"launch_type", launchType},
            {"app_version", info.version},
            {"unique_id", uniqueID},
            {"errors", errors},
            {"app_package_type", info.pakType},
        },
    });
#else
    Q_UNUSED(appName)
    Q_UNUSED(errors)
    Q_UNUSED(launchType)
    Q_UNUSED(uniqueID)
#endif
}

void EventReporter::reportAppAbnormalExit(const QString &appName, const QString &launchType, const QString &exec, const QString &logInfo, bool isLinglong, const QString &uniqueID)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    auto info = queryAppPackageInfo(appName, isLinglong);
    DDE_EventLogger::EventLogger::instance().writeEventLog({
        1000600012,
        appName,
        QJsonObject{
            {"app_name", appName},
            {"launch_type", launchType},
            {"unique_id", uniqueID},
            {"exec", exec},
            {"log", logInfo},
            {"app_package_type", info.pakType},
        },
    });
#else
    Q_UNUSED(appName)
    Q_UNUSED(launchType)
    Q_UNUSED(exec)
    Q_UNUSED(logInfo)
    Q_UNUSED(uniqueID)
#endif
}
