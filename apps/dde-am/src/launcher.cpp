// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "launcher.h"
#include "global.h"

#include <DConfig>
#include <QDBusConnection>
#include <QDBusMetaType>

DCORE_USE_NAMESPACE

namespace {
void registerComplexDbusType()
{
    qRegisterMetaType<ObjectInterfaceMap>();
    qDBusRegisterMetaType<ObjectInterfaceMap>();
    qRegisterMetaType<ObjectMap>();
    qDBusRegisterMetaType<ObjectMap>();
    qDBusRegisterMetaType<QStringMap>();
    qRegisterMetaType<QStringMap>();
    qRegisterMetaType<PropMap>();
    qDBusRegisterMetaType<PropMap>();
    qDBusRegisterMetaType<QDBusObjectPath>();
}

template <class T>
DExpected<T> parseDBusField(const QVariantMap &map, const QString &key)
{
    if (!map.contains(key)) {
        return DUnexpected{emplace_tag::USE_EMPLACE, -1, QString("%1 doesn't exist.").arg(key)};
    }

    const auto value = map.value(key);
    return DExpected<T>{qdbus_cast<T>(value)};
}

ObjectMap getManagedObjects()
{
    registerComplexDbusType();

    auto msg = QDBusMessage::createMethodCall(
        DDEApplicationManager1ServiceName, DDEApplicationManager1ObjectPath, ObjectManagerInterface, "GetManagedObjects");

    auto reply = QDBusConnection::sessionBus().call(msg);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        qFatal() << "Failed to fetch application infos" << reply.errorMessage();
    }
    const auto &arguments = reply.arguments();
    Q_ASSERT_X(!arguments.isEmpty(), "", "Incorrect reply argument for GetManagedObjects call.");

    return qdbus_cast<ObjectMap>(arguments.first());
}
}  // namespace

DExpected<QStringList> Launcher::appIds()
{
    QStringList appIds;
    const auto objects = getManagedObjects();
    for (auto iter = objects.cbegin(); iter != objects.cend(); ++iter) {
        const auto &objPath = iter.key().path();
        const ObjectInterfaceMap &objs = iter.value();
        const QVariantMap appInfo = objs.value("org.desktopspec.ApplicationManager1.Application");
        if (appInfo.isEmpty()) {
            continue;
        }

        if (auto value = parseDBusField<QString>(appInfo, u8"ID")) {
            appIds.append(value.value());
        } else {
            qFatal() << "Failed to parse application ID";
        }
    }

    return appIds;
}

void Launcher::setPath(const QString &path)
{
    m_path = path;
}

void Launcher::setAction(const QString &action)
{
    m_action = action;
}

void Launcher::setLaunchedType(Type type)
{
    m_launchedType = type;
}

void Launcher::setEnvironmentVariables(const QStringList &envVars)
{
    m_environmentVariables = envVars;
}

DExpected<void> Launcher::run()
{
    if (auto value = launch(); !value) {
        return value;
    }

    if (m_launchedType == Type::ByUser) {
        if (auto ret = updateLaunchedTimes(); !ret) {
            return ret;
        }
    }

    return {};
}

DExpected<void> Launcher::launch() noexcept
{
    // Build options map
    QVariantMap options;
    if (!m_environmentVariables.isEmpty()) {
        options.insert("env", m_environmentVariables);
    }

    // Mark autostart launches so AM can suppress splash
    if (m_autostart) {
        options.insert("_autostart", true);
    }

    auto msg = QDBusMessage::createMethodCall(DDEApplicationManager1ServiceName, m_path, ApplicationInterface, "Launch");
    msg.setArguments({m_action, QStringList{}, options});
    auto reply = QDBusConnection::sessionBus().call(msg);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        return DUnexpected{emplace_tag::USE_EMPLACE,
                           static_cast<int>(reply.type()),
                           QString("Failed to launch: %1, error: %2").arg(appId(), reply.errorMessage())};
    }

    return {};
}

DExpected<void> Launcher::updateLaunchedTimes() noexcept
{
    std::unique_ptr<DConfig> config(DConfig::create(
        // use QString::fromRawData(const char16_t*, size) if Qt version >= 6.10
        QString::fromRawData(reinterpret_cast<const QChar *>(ApplicationServiceID), std::size(ApplicationServiceID) - 1),
        ApplicationManagerToolsConfig));
    if (!config->isValid()) {
        return DUnexpected{emplace_tag::USE_EMPLACE, -1, "DConfig is invalid when updating launched times."};
    }

    using namespace Qt::StringLiterals;
    const QString AppsLaunchedTimes(u"appsLaunchedTimes"_s);
    QVariantMap launchedTimes = config->value(AppsLaunchedTimes).toMap();
    const auto appKey = appId();
    if (appKey.isEmpty()) {
        return DUnexpected{emplace_tag::USE_EMPLACE, -1, QString("Empty appId for the path: %1").arg(m_path)};
    }

    auto it = launchedTimes.find(appKey);
    if (it != launchedTimes.end()) {
        it.value() = it->value<qlonglong>() + 1;
    } else {
        launchedTimes.insert(appKey, static_cast<qlonglong>(1));
    }

    config->setValue(AppsLaunchedTimes, launchedTimes);
    return {};
}

QString Launcher::appId() const noexcept
{
    if (m_path.isEmpty()) {
        return {};
    }

    const auto startIndex = QString(DDEApplicationManager1ObjectPath).size();
    auto endIndex = m_path.indexOf("/", startIndex + 1);
    const auto id = endIndex <= -1 ? m_path.mid(startIndex + 1) : m_path.sliced(startIndex + 1, endIndex - (startIndex + 1));
    return unescapeFromObjectPath(id);
}

void Launcher::setAutostart(bool autostart)
{
    m_autostart = autostart;
}
