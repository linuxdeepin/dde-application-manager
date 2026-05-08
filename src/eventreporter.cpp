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
}

void EventReporter::reportAppLaunch(const QString &appName, const QString &appVersion, qint64 timeMs, const QString &launchType, const QString &uniqueID)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    DDE_EventLogger::EventLogger::instance().writeEventLog({
        1000610001,
        appName,
        QJsonObject{
            {"app_name", appName},
            {"launch_type", launchType},
            {"app_version", appVersion},
            {"unique_id", uniqueID},
            {"time", timeMs},
        },
    });
#else
    Q_UNUSED(appName)
    Q_UNUSED(appVersion)
    Q_UNUSED(timeMs)
    Q_UNUSED(launchType)
    Q_UNUSED(uniqueID)
#endif
}

void EventReporter::reportAppLaunchFailed(const QString &appName, const QString &appVersion, const QString &errors, const QString &launchType, const QString &uniqueID)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    DDE_EventLogger::EventLogger::instance().writeEventLog({
        1000610002,
        appName,
        QJsonObject{
            {"app_name", appName},
            {"launch_type", launchType},
            {"app_version", appVersion},
            {"unique_id", uniqueID},
            {"errors", errors},
        },
    });
#else
    Q_UNUSED(appName)
    Q_UNUSED(appVersion)
    Q_UNUSED(errors)
    Q_UNUSED(launchType)
    Q_UNUSED(uniqueID)
#endif
}

void EventReporter::reportAppLaunchDuration(const QString &appName, const QString &appVersion, qint64 timeMs, const QString &launchType, const QString &uniqueID)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    DDE_EventLogger::EventLogger::instance().writeEventLog({
        1000610003,
        appName,
        QJsonObject{
            {"app_name", appName},
            {"launch_type", launchType},
            {"app_version", appVersion},
            {"unique_id", uniqueID},
            {"time", timeMs},
        },
    });
#else
    Q_UNUSED(appName)
    Q_UNUSED(appVersion)
    Q_UNUSED(timeMs)
    Q_UNUSED(launchType)
    Q_UNUSED(uniqueID)
#endif
}

void EventReporter::reportAppAbnormalExit(const QString &appName, const QString &launchType, const QString &exec, const QString &logInfo, const QString &uniqueID)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    DDE_EventLogger::EventLogger::instance().writeEventLog({
        1000600012,
        appName,
        QJsonObject{
            {"app_name", appName},
            {"launch_type", launchType},
            {"unique_id", uniqueID},
            {"exec", exec},
            {"log", logInfo},
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

QString EventReporter::getAppVersion(const QString &appId)
{
    auto &cache = versionCache();
    if (auto it = cache.constFind(appId); it != cache.constEnd()) {
        return it.value();
    }

    qCDebug(amEventReporter) << "query version for appId:" << appId;
    QProcess proc;
    proc.start("dpkg-query", {"-W", "-f=${Version}", appId});
    proc.waitForFinished(3000);
    QString version;
    if (proc.exitCode() == 0) {
        version = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    }

    if (version.isEmpty() && appId.startsWith("org.")) {
        proc.start("ll-cli", {"info", appId});
        proc.waitForFinished(5000);
        if (proc.exitCode() == 0) {
            auto doc = QJsonDocument::fromJson(proc.readAllStandardOutput());
            if (doc.isObject()) {
                version = doc.object().value("version").toString();
            }
        }
    }

    if (!version.isEmpty()) {
        cache.insert(appId, version);
    }

    return version;
}
