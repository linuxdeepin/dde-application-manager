// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "cgroupthp.h"
#include "config.h"
#include "constant.h"
#include "global.h"

#include <DConfig>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QFile>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>

#include <memory>
#include <optional>

using namespace Qt::StringLiterals;

DCORE_USE_NAMESPACE

Q_LOGGING_CATEGORY(logCgroupThp, "dde.am.thp")

namespace {

const auto DefaultMode = u"force_enable"_s;

// Avoid flooding the log with the same problem on every launch.
bool shouldWarn(const QString &key)
{
    static QMutex mutex;
    static QSet<QString> warned;

    QMutexLocker locker(&mutex);
    if (warned.contains(key)) {
        return false;
    }
    if (warned.size() > 1024) {
        warned.clear();
    }
    warned.insert(key);
    return true;
}

std::optional<QString> modeFromConfig(const QString &appId)
{
    std::unique_ptr<DConfig> config(DConfig::create(fromStaticRaw(ApplicationServiceID),
                                                    fromStaticRaw(ApplicationManagerConfig),
                                                    u"/"_s % appId));
    if (!config || !config->isValid()) {
        if (shouldWarn(u"unavailable:"_s % appId)) {
            qCWarning(logCgroupThp) << "DConfig is unavailable, skip applying THP mode for" << appId;
        }
        return std::nullopt;
    }

    const auto key = fromStaticRaw(AppThpMode);
    if (!config->keyList().contains(key)) {
        if (shouldWarn(u"missing:"_s % appId)) {
            qCWarning(logCgroupThp) << "THP mode is not defined in the configuration, skip applying it for" << appId;
        }
        return std::nullopt;
    }

    const auto value = config->value(key);
    if (!value.isValid()) {
        if (shouldWarn(u"missing:"_s % appId)) {
            qCWarning(logCgroupThp) << "THP mode is not defined in the configuration, skip applying it for" << appId;
        }
        return std::nullopt;
    }

    const auto mode = value.toString();
    if (!CgroupThp::isValidMode(mode)) {
        if (shouldWarn(u"invalid:"_s % appId % u":"_s % mode)) {
            qCWarning(logCgroupThp) << "Invalid THP mode" << mode << "for" << appId
                                    << ", falling back to" << DefaultMode;
        }
        return DefaultMode;
    }

    return mode;
}

std::optional<QString> controlGroupOfUnit(const QString &unitName)
{
    auto unitMsg = QDBusMessage::createMethodCall(QString::fromUtf8(SystemdService),
                                                  QString::fromUtf8(SystemdObjectPath),
                                                  QString::fromUtf8(SystemdInterfaceName),
                                                  u"GetUnit"_s);
    unitMsg << unitName;

    const auto unitReply = QDBusConnection::sessionBus().call(unitMsg, QDBus::Block, 5000);
    if (unitReply.type() != QDBusMessage::ReplyMessage || unitReply.arguments().isEmpty()) {
        qCDebug(logCgroupThp) << "Failed to resolve systemd unit" << unitName << ":" << unitReply.errorMessage();
        return std::nullopt;
    }

    const auto unitPath = unitReply.arguments().constFirst().value<QDBusObjectPath>().path();
    if (unitPath.isEmpty()) {
        qCDebug(logCgroupThp) << "Empty object path for systemd unit" << unitName;
        return std::nullopt;
    }

    auto propertyMsg = QDBusMessage::createMethodCall(QString::fromUtf8(SystemdService),
                                                      unitPath,
                                                      fromStaticRaw(SystemdPropInterfaceName),
                                                      u"Get"_s);
    propertyMsg << fromStaticRaw(SystemdServiceInterfaceName);
    propertyMsg << u"ControlGroup"_s;

    const auto propertyReply = QDBusConnection::sessionBus().call(propertyMsg, QDBus::Block, 5000);
    if (propertyReply.type() != QDBusMessage::ReplyMessage || propertyReply.arguments().isEmpty()) {
        qCDebug(logCgroupThp) << "Failed to read ControlGroup of" << unitName << ":" << propertyReply.errorMessage();
        return std::nullopt;
    }

    const auto controlGroup = propertyReply.arguments().constFirst().value<QDBusVariant>().variant().toString();
    if (controlGroup.isEmpty()) {
        qCDebug(logCgroupThp) << "No control group (yet) for systemd unit" << unitName;
        return std::nullopt;
    }

    return controlGroup;
}

void writeMode(const QString &cgroupPath, const QString &mode)
{
    const auto filePath = cgroupPath % u"/memory.thp_mode"_s;
    QFile file(filePath);
    if (!file.exists()) {
        qCDebug(logCgroupThp) << "memory.thp_mode is not available at" << filePath << ", skip";
        return;
    }

    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(logCgroupThp) << "Failed to open" << filePath << ":" << file.errorString();
        return;
    }

    const auto payload = mode.toUtf8();
    if (file.write(payload) != payload.size()) {
        qCWarning(logCgroupThp) << "Failed to write THP mode" << mode << "to" << filePath << ":" << file.errorString();
        return;
    }

    qCInfo(logCgroupThp) << "Applied THP mode" << mode << "to" << filePath;
}

}  // namespace

namespace CgroupThp {

bool isValidMode(QStringView mode) noexcept
{
    const auto value = mode.toString();
    return value == u"inherit"_s || value == u"default"_s || value == u"disable"_s || value == u"force_enable"_s;
}

void applyForUnit(const QString &unitName, const QString &appId) noexcept
{
    const auto mode = modeFromConfig(appId);
    if (!mode.has_value()) {
        return;
    }

    const auto controlGroup = controlGroupOfUnit(unitName);
    if (!controlGroup.has_value()) {
        return;
    }

    writeMode(u"/sys/fs/cgroup"_s % *controlGroup, *mode);
}

}  // namespace CgroupThp
