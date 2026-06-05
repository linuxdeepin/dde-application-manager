// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "eventreporter.h"
#include "config.h"
#include "constant.h"
#include "global.h"

#ifdef HAVE_DDE_API_EVENTLOGGER
#include <dde-api/eventlogger.hpp>
#endif
#include <DConfig>

#include <QLoggingCategory>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QHash>
#include <algorithm>
#include <memory>

Q_LOGGING_CATEGORY(amEventReporter, "dde.am.event.reporter")

EventReporter &EventReporter::instance()
{
    static EventReporter reporter;
    return reporter;
}

void EventReporter::initialize()
{
    DCORE_USE_NAMESPACE
    std::unique_ptr<DConfig> config(DConfig::create(
        fromStaticRaw(ApplicationServiceID),
        fromStaticRaw(ApplicationManagerConfig)));

    if (!config || !config->isValid()) {
        qCInfo(amEventReporter) << "DConfig not available, skip event filter disabled.";
        return;
    }

    m_skipEventAppIds = config->value(fromStaticRaw(SkipEventAppIds)).toStringList();
    qCInfo(amEventReporter) << "skip event appIds:" << m_skipEventAppIds;
}

bool EventReporter::shouldSkip(const QString &appId) const
{
    if (appId.isEmpty())
        return false;

    if (m_skipEventAppIds.contains(appId)) {
        qCDebug(amEventReporter) << "skip all events for appId:" << appId;
        return true;
    }

    return false;
}

EventReporter::CacheEntry EventReporter::queryAppPackageInfo(const QString &appId, bool isLinglong)
{
    auto cacheIt = m_cache.constFind(appId);
    if (cacheIt != m_cache.constEnd()) {
        qint64 age = QDateTime::currentMSecsSinceEpoch() - cacheIt->timestamp;
        if (age < kCacheTimeoutMs) {
            m_cache[appId].timestamp = QDateTime::currentMSecsSinceEpoch();
            return *cacheIt;
        }
        qCDebug(amEventReporter) << "cache stale for appId:" << appId << "age:" << age << "ms";
    }

    CacheEntry info;
    if (cacheIt != m_cache.constEnd()) {
        info.version = cacheIt->version;
        info.pakType = cacheIt->pakType;
    }

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

    info.timestamp = QDateTime::currentMSecsSinceEpoch();

    // LRU eviction: if it's a new entry and at capacity, remove the oldest
    if (cacheIt == m_cache.constEnd() && m_cache.size() >= kMaxCacheSize) {
        auto oldest = std::min_element(m_cache.constKeyValueBegin(),
                                       m_cache.constKeyValueEnd(),
                                       [](const auto &a, const auto &b) {
                                           return a.second.timestamp < b.second.timestamp;
                                       });
        if (oldest != m_cache.constKeyValueEnd()) {
            qCDebug(amEventReporter) << "evict cache for appId:" << oldest->first;
            m_cache.remove(oldest->first);
        }
    }

    m_cache.insert(appId, info);
    return info;
}

void EventReporter::reportAppLaunch(const QString &appName, qint64 timeMs, bool isLinglong, const QString &launchType, const QString &uniqueID)
{
#ifdef HAVE_DDE_API_EVENTLOGGER
    if (shouldSkip(appName))
        return;

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
    if (shouldSkip(appName))
        return;

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
    if (shouldSkip(appName))
        return;

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
