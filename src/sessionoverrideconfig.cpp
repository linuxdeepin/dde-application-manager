// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "sessionoverrideconfig.h"
#include "constant.h"
#include "global.h"
#include "sessiontype.h"

#include <DConfig>
#include <QLoggingCategory>

using namespace Qt::StringLiterals;
using DTK_CORE_NAMESPACE::DConfig;

Q_LOGGING_CATEGORY(logSessionOverride, "dde.am.session.override")

class SessionOverrideDiscovery : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    void refresh()
    {
        if (!m_root) {
            auto *config = DConfig::create(fromStaticRaw(ApplicationServiceID),
                                           fromStaticRaw(ApplicationOverrideConfigResource), {}, this);
            if (!config->isValid()) {
                qCWarning(logSessionOverride) << "Failed to load root DConfig"
                                              << fromStaticRaw(ApplicationOverrideConfigResource);
                delete config;
                return;
            }
            connect(config, &DConfig::subpathsChanged, this, &SessionOverrideDiscovery::refresh);
            m_root = config;
        }
        if (m_root->isValid())
            emit subpathsRead(m_root->subpaths());
    }

signals:
    void subpathsRead(const QStringList &subpaths);

private:
    DConfig *m_root = nullptr;
};

SessionOverrideConfig::SessionOverrideConfig(QObject *parent)
    : QObject(parent)
{
    switch (currentSessionType()) {
    case SessionType::Wayland:
        m_subpathPrefix = u"/wayland"_s;
        break;
    case SessionType::X11:
        m_subpathPrefix = u"/x11"_s;
        break;
    case SessionType::Unknown:
        qCInfo(logSessionOverride) << "Unknown session type, session overrides disabled.";
        return;
    }

    qCInfo(logSessionOverride) << "Discovering session overrides for" << m_subpathPrefix;
    auto *discovery = new SessionOverrideDiscovery(this);
    connect(discovery, &SessionOverrideDiscovery::subpathsRead, this, &SessionOverrideConfig::applySubpaths);
    discovery->refresh();
}

SessionOverrideConfig::~SessionOverrideConfig() = default;

void SessionOverrideConfig::applySubpaths(const QStringList &subpaths)
{
    qCDebug(logSessionOverride) << "Read" << subpaths.size() << "override subpaths for" << m_subpathPrefix;
    m_subpaths = QSet<QString>(subpaths.cbegin(), subpaths.cend());
    for (auto it = m_configs.begin(); it != m_configs.end();) {
        if (!m_subpaths.contains(u"/"_s % it->first % m_subpathPrefix))
            it = m_configs.erase(it);
        else
            ++it;
    }
    for (const auto &id : m_overrides.keys()) {
        if (!m_subpaths.contains(u"/"_s % id % m_subpathPrefix))
            setOverride(id, {});
    }
    for (const auto &id : std::as_const(m_requested))
        configFor(id);
}

void SessionOverrideConfig::setOverride(const QString &desktopId, const QHash<QString, QVariant> &values)
{
    const auto previous = m_overrides.value(desktopId);
    if (previous == values)
        return;
    if (values.isEmpty())
        m_overrides.remove(desktopId);
    else
        m_overrides[desktopId] = values;
    for (const auto &key : {u"Exec"_s, u"TryExec"_s, u"Icon"_s}) {
        if (previous.value(key) != values.value(key))
            emit overrideChanged(desktopId, key);
    }
    emit configChanged();
}

void SessionOverrideConfig::preload(const QStringList &desktopIds)
{
    for (const auto &id : desktopIds) {
        ensureLoaded(id);
    }
}

void SessionOverrideConfig::ensureLoaded(const QString &desktopId) const
{
    if (m_subpathPrefix.isEmpty())
        return;
    m_requested.insert(desktopId);
    configFor(desktopId);
}

void SessionOverrideConfig::updateOverride(const QString &desktopId) const
{
    const auto it = m_configs.find(desktopId);
    if (it == m_configs.end())
        return;
    const auto *config = it->second.get();

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

    const_cast<SessionOverrideConfig *>(this)->setOverride(desktopId, values);
}

ApplicationOverrideConfig *SessionOverrideConfig::configFor(const QString &desktopId) const
{
    const auto subpath = u"/"_s % desktopId % m_subpathPrefix;
    if (!m_subpaths.contains(subpath))
        return nullptr;

    auto it = m_configs.find(desktopId);
    if (it != m_configs.end())
        return it->second.get();

    auto *config = ApplicationOverrideConfig::create(fromStaticRaw(ApplicationServiceID),
                                                     subpath,
                                                     const_cast<SessionOverrideConfig*>(this));
    if (!config) {
        qCWarning(logSessionOverride) << "Failed to create DConfig for subpath:" << subpath;
        return nullptr;
    }

    qCDebug(logSessionOverride) << "Creating DConfig for" << desktopId << "subpath:" << subpath;

    auto *self = const_cast<SessionOverrideConfig*>(this);
    const QPointer<ApplicationOverrideConfig> safeConfig(config);
    QObject::connect(config, &ApplicationOverrideConfig::configInitializeSucceed,
                     self, [self, desktopId, safeConfig](DTK_CORE_NAMESPACE::DConfig *) {
        if (!safeConfig)
            return;
        QObject::connect(safeConfig.data(), &ApplicationOverrideConfig::valueChanged,
                         self, [self, desktopId, safeConfig](const QString &key) {
            if (safeConfig && (key == u"Exec"_s || key == u"TryExec"_s || key == u"Icon"_s))
                self->updateOverride(desktopId);
        });
        self->updateOverride(desktopId);
    });

    QObject::connect(config, &ApplicationOverrideConfig::configInitializeFailed,
                     self, [subpath]() {
        qCWarning(logSessionOverride) << "Override config initialization failed for subpath" << subpath;
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

#include "sessionoverrideconfig.moc"
