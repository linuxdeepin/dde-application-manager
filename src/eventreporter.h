// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class EventReporter
{
public:
    static EventReporter &instance();

    void initialize();
    void reportAppLaunch(const QString &appName, qint64 timeMs, bool isLinglong, const QString &launchType = {}, const QString &uniqueID = {});
    void reportAppLaunchFailed(const QString &appName, const QString &errors, bool isLinglong, const QString &launchType = {}, const QString &uniqueID = {});
    void reportAppAbnormalExit(const QString &appName, const QString &launchType, const QString &exec, const QString &logInfo, bool isLinglong, const QString &uniqueID = {});

private:
    EventReporter() = default;

    struct CacheEntry {
        QString version;
        QString pakType;
        qint64 timestamp = 0;
    };

    bool shouldSkip(const QString &appId) const;
    CacheEntry queryAppPackageInfo(const QString &appId, bool isLinglong);

    QStringList m_skipEventAppIds;
    QHash<QString, CacheEntry> m_cache;

    static constexpr int kMaxCacheSize = 128;
    static constexpr qint64 kCacheTimeoutMs = 600'000;  // 10 minutes
};
