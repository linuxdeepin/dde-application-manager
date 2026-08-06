// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "sessionoverrideconfig.h"
#include "constant.h"
#include "global.h"
#include "sessiontype.h"

#include <QLoggingCategory>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(logSessionOverride, "dde.am.session.override")

SessionOverrideConfig::SessionOverrideConfig(QObject *parent)
    : QObject(parent)
{
    m_sessionType = currentSessionType();
    switch (m_sessionType) {
    case SessionType::Wayland:
        m_subpathPrefix = u"/wayland"_s;
        break;
    case SessionType::X11:
        m_subpathPrefix = u"/x11"_s;
        break;
    case SessionType::Unknown:
        qCInfo(logSessionOverride) << "Unknown session type, session overrides disabled.";
        break;
    }
    qCInfo(logSessionOverride) << "Session override config initialized for" << m_subpathPrefix;
}

SessionOverrideConfig::~SessionOverrideConfig() = default;

void SessionOverrideConfig::preload(const QStringList &desktopIds)
{
    for (const auto &id : desktopIds) {
        ensureLoaded(id);
    }
}

void SessionOverrideConfig::ensureLoaded(const QString &desktopId) const
{
    if (m_loaded.contains(desktopId))
        return;
    m_loaded.insert(desktopId);

    configFor(desktopId);
}

void SessionOverrideConfig::updateOverride(const QString &desktopId) const
{
    auto *config = configFor(desktopId);
    if (!config)
        return;

    QHash<QString, QVariant> values;

    const auto exec = config->Exec();
    if (!exec.isEmpty()) {
        values[u"Exec"_s] = exec;
    }

    const auto tryExec = config->TryExec();
    if (!tryExec.isEmpty()) {
        values[u"TryExec"_s] = tryExec;
    }

    const auto icon = config->Icon();
    if (!icon.isEmpty()) {
        values[u"Icon"_s] = icon;
    }

    if (!values.isEmpty()) {
        m_overrides[desktopId] = values;
    } else {
        m_overrides.remove(desktopId);
    }
}

ApplicationOverrideConfig *SessionOverrideConfig::configFor(const QString &desktopId) const
{
    if (m_subpathPrefix.isEmpty())
        return nullptr;

    auto it = m_configs.find(desktopId);
    if (it != m_configs.end())
        return it->second.get();

    const auto subpath = u"/"_s % desktopId % m_subpathPrefix;
    auto *config = ApplicationOverrideConfig::create(fromStaticRaw(ApplicationServiceID),
                                                     subpath,
                                                     const_cast<SessionOverrideConfig*>(this));
    if (!config) {
        qCWarning(logSessionOverride) << "Failed to create DConfig for subpath:" << subpath;
        return nullptr;
    }

    qCInfo(logSessionOverride) << "Creating DConfig for" << desktopId << "subpath:" << subpath;

    auto *self = const_cast<SessionOverrideConfig*>(this);
    QObject::connect(config, &ApplicationOverrideConfig::valueChanged,
                     self, [self, desktopId](const QString &key) {
        if (key == u"Exec"_s || key == u"TryExec"_s || key == u"Icon"_s) {
            qCInfo(logSessionOverride) << "Override changed for" << desktopId << "key:" << key;
            self->updateOverride(desktopId);
            emit self->overrideChanged(desktopId, key);
            emit self->configChanged();
        }
    });

    QObject::connect(config, &ApplicationOverrideConfig::configInitializeSucceed,
                     self, [self, desktopId, config](DTK_CORE_NAMESPACE::DConfig *) {
        qCInfo(logSessionOverride) << "Override config initialized for" << desktopId;
        self->updateOverride(desktopId);
        qCInfo(logSessionOverride) << "After init override for" << desktopId
                                   << "Exec:" << config->Exec()
                                   << "Icon:" << config->Icon()
                                   << "TryExec:" << config->TryExec()
                                   << "hasOverride:" << self->hasOverride(desktopId);
    });

    QObject::connect(config, &ApplicationOverrideConfig::configInitializeFailed,
                     self, [self, desktopId]() {
        qCWarning(logSessionOverride) << "Override config initialization failed for" << desktopId;
    });

    m_configs[desktopId] = std::unique_ptr<ApplicationOverrideConfig>(config);
    return config;
}

std::optional<QString> SessionOverrideConfig::getValue(const QString &desktopId,
                                                       const QString &group,
                                                       const QString &field) const
{
    if (group != fromStaticRaw(DesktopFileEntryKey)) {
        return std::nullopt;
    }

    ensureLoaded(desktopId);

    auto desktopIt = m_overrides.constFind(desktopId);
    if (desktopIt != m_overrides.constEnd()) {
        auto fieldIt = desktopIt->constFind(field);
        if (fieldIt != desktopIt->constEnd()) {
            return fieldIt->toString();
        }
        return std::nullopt;
    }

    auto *config = configFor(desktopId);
    if (!config)
        return std::nullopt;

    if (field == u"Exec"_s) {
        const auto exec = config->Exec();
        if (exec.isEmpty())
            return std::nullopt;
        return exec;
    }
    if (field == u"TryExec"_s) {
        const auto tryExec = config->TryExec();
        if (tryExec.isEmpty())
            return std::nullopt;
        return tryExec;
    }
    if (field == u"Icon"_s) {
        const auto icon = config->Icon();
        if (icon.isEmpty())
            return std::nullopt;
        return icon;
    }
    return std::nullopt;
}

bool SessionOverrideConfig::hasOverride(const QString &desktopId) const
{
    ensureLoaded(desktopId);
    return m_overrides.contains(desktopId);
}

QString SessionOverrideConfig::resolveExecValue(const QString &overrideValue, QStringView originalExec)
{
    QString ret = overrideValue;
    if (ret.contains(u"!AM_FULL!"_s)) {
        ret.replace(u"!AM_FULL!"_s, originalExec.toString());
    }
    return ret;
}