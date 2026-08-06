// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SESSIONOVERRIDECONFIG_H
#define SESSIONOVERRIDECONFIG_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <memory>
#include <optional>
#include <QSet>
#include <map>

#include "am_appoverride_config.hpp"
#include "sessiontype.h"

class SessionOverrideConfig : public QObject
{
    Q_OBJECT
public:
    explicit SessionOverrideConfig(QObject *parent = nullptr);
    ~SessionOverrideConfig() override;

    std::optional<QString> getValue(const QString &desktopId, const QString &group, const QString &field) const;
    bool hasOverride(const QString &desktopId) const;
    void preload(const QStringList &desktopIds);

    static QString resolveExecValue(const QString &overrideValue, QStringView originalExec);

signals:
    void configChanged();
    void overrideChanged(const QString &desktopId, const QString &key);

private:
    Q_DISABLE_COPY(SessionOverrideConfig)
    void ensureLoaded(const QString &desktopId) const;
    void updateOverride(const QString &desktopId) const;
    ApplicationOverrideConfig *configFor(const QString &desktopId) const;

    mutable std::map<QString, std::unique_ptr<ApplicationOverrideConfig>> m_configs;
    mutable QHash<QString, QHash<QString, QVariant>> m_overrides;
    mutable QSet<QString> m_loaded;
    SessionType m_sessionType;
    QString m_subpathPrefix;
};

#endif