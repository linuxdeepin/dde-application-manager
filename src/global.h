// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef GLOBAL_H
#define GLOBAL_H

#include "config.h"
#include "constant.h"
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QStandardPaths>
#include <QDir>
#include <QLocale>
#include <QLoggingCategory>
#include <QMap>
#include <QMetaClassInfo>
#include <QString>
#include <QUuid>
#include <csignal>  // IWYU pragma: keep
#include <optional>
#include <sys/syscall.h>
#include <unistd.h>
#include <QDBusAbstractAdaptor>

Q_DECLARE_LOGGING_CATEGORY(DDEAMProf)
Q_DECLARE_LOGGING_CATEGORY(DDEAMUtils)

using ObjectInterfaceMap = QMap<QString, QVariantMap>;
using ObjectMap = QMap<QDBusObjectPath, ObjectInterfaceMap>;
using QStringMap = QMap<QString, QString>;
using PropMap = QMap<QString, QStringMap>;

Q_DECLARE_METATYPE(ObjectInterfaceMap)
Q_DECLARE_METATYPE(ObjectMap)
Q_DECLARE_METATYPE(QStringMap)
Q_DECLARE_METATYPE(PropMap)

struct SystemdUnitDBusMessage
{
    QString name;
    QString subState;
    QDBusObjectPath objectPath;
};

// --- Systemd D-Bus 类型定义开始 ---

// 对应 Systemd ExecStart 的结构: (path, argv, ignore_failure) -> (sasb)
struct SystemdExecCommand
{
    QString path;
    QStringList args;
    bool unclean;  // 是否忽略错误
};
Q_DECLARE_METATYPE(SystemdExecCommand)

// 对应 Systemd 属性结构: (name, value) -> (sv)
struct SystemdProperty
{
    QString name;
    QDBusVariant value;  // 使用 QDBusVariant 来明确这是一个 Variant 容器
};
Q_DECLARE_METATYPE(SystemdProperty)

// 对应 Systemd 辅助单元结构 (Aux Unit): (name, properties) -> (sa(sv))
// StartTransientUnit 的第4个参数需要这个类型的数组，即使它是空的
struct SystemdAux
{
    QString name;
    QList<SystemdProperty> properties;
};
Q_DECLARE_METATYPE(SystemdAux)

// 序列化操作符重载
inline QDBusArgument &operator<<(QDBusArgument &arg, const SystemdExecCommand &cmd)
{
    arg.beginStructure();
    arg << cmd.path << cmd.args << cmd.unclean;
    arg.endStructure();
    return arg;
}

inline const QDBusArgument &operator>>(const QDBusArgument &arg, SystemdExecCommand &cmd)
{
    arg.beginStructure();
    arg >> cmd.path >> cmd.args >> cmd.unclean;
    arg.endStructure();
    return arg;
}

inline QDBusArgument &operator<<(QDBusArgument &arg, const SystemdProperty &prop)
{
    arg.beginStructure();
    arg << prop.name << prop.value;
    arg.endStructure();
    return arg;
}

inline const QDBusArgument &operator>>(const QDBusArgument &arg, SystemdProperty &prop)
{
    arg.beginStructure();
    arg >> prop.name >> prop.value;
    arg.endStructure();
    return arg;
}

inline QDBusArgument &operator<<(QDBusArgument &arg, const SystemdAux &aux)
{
    arg.beginStructure();
    arg << aux.name << aux.properties;
    arg.endStructure();
    return arg;
}

inline const QDBusArgument &operator>>(const QDBusArgument &arg, SystemdAux &aux)
{
    arg.beginStructure();
    arg >> aux.name >> aux.properties;
    arg.endStructure();
    return arg;
}
// --- Systemd D-Bus 类型定义结束 ---

inline const QDBusArgument &operator>>(const QDBusArgument &argument, QStringMap &map)
{
    argument.beginMap();
    while (!argument.atEnd()) {
        QString key;
        QString value;
        argument.beginMapEntry();
        argument >> key >> value;
        argument.endMapEntry();
        map.insert(key, value);
    }
    argument.endMap();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, QList<SystemdUnitDBusMessage> &units)
{
    argument.beginArray();
    while (!argument.atEnd()) {
        argument.beginStructure();
        QString _str;
        uint32_t _uint;
        QDBusObjectPath _path;
        SystemdUnitDBusMessage unit;
        argument >> unit.name >> _str >> _str >> _str >> unit.subState >> _str >> unit.objectPath >> _uint >> _str >> _path;
        units.emplace_back(std::move(unit));
        argument.endStructure();
    }
    argument.endArray();

    return argument;
}

inline const QString &getApplicationLauncherBinary() noexcept
{
    static const auto &bin = []() {
        if (auto value = qEnvironmentVariable("DEEPIN_APPLICATION_MANAGER_APP_LAUNCH_HELPER_BIN"); !value.isEmpty()) {
            qCWarning(DDEAMUtils)
                << "Using app launch helper defined in environment variable DEEPIN_APPLICATION_MANAGER_APP_LAUNCH_HELPER_BIN:"
                << value;
            return value;
        }

        return QString::fromRawData(reinterpret_cast<const QChar *>(ApplicationLaunchHelperBinary),
                                    std::size(ApplicationLaunchHelperBinary) - 1);
    }();

    return bin;
}

enum class DBusType : uint8_t { Session = QDBusConnection::SessionBus, System = QDBusConnection::SystemBus, Custom = 2 };

template <typename T>
using remove_cvr_t = std::remove_reference_t<std::remove_cv_t<T>>;

template <typename T>
void applyIteratively(QList<QDir> dirs,
                      T func,
                      QDir::Filters filter = QDir::NoFilter,
                      const QStringList &nameFilter = {},
                      QDir::SortFlags sortFlag = QDir::SortFlag::NoSort) noexcept
{
    static_assert(std::is_invocable_v<T, const QFileInfo &>, "apply function should only accept one QFileInfo");
    static_assert(std::is_same_v<decltype(func(QFileInfo{})), bool>,
                  "apply function should return a boolean to indicate when should return");
    QList<QDir> dirList{std::move(dirs)};

    while (!dirList.isEmpty()) {
        const auto &dir = dirList.takeFirst();

        if (!dir.exists()) {
            qCWarning(DDEAMUtils) << "apply function to an non-existent directory:" << dir.absolutePath() << ", skip.";
            continue;
        }
        const auto &infoList = dir.entryInfoList(nameFilter, filter, sortFlag);

        for (const auto &info : infoList) {
            if (info.isFile() && func(info)) {
                return;
            }

            if (info.isDir()) {
                dirList.append(info.absoluteFilePath());
            }
        }
    }
}

template <std::size_t N>
inline QString fromStaticRaw(const char16_t (&array)[N]) noexcept
{
    return QString::fromRawData(reinterpret_cast<const QChar *>(array), static_cast<qsizetype>(N - 1));
}

class ApplicationManager1DBus
{
public:
    ApplicationManager1DBus(const ApplicationManager1DBus &) = delete;
    ApplicationManager1DBus(ApplicationManager1DBus &&) = delete;
    ApplicationManager1DBus &operator=(const ApplicationManager1DBus &) = delete;
    ApplicationManager1DBus &operator=(ApplicationManager1DBus &&) = delete;
    [[nodiscard]] const QString &globalDestBusAddress() const noexcept { return m_destBusAddress; }
    [[nodiscard]] const QString &globalServerBusAddress() const noexcept { return m_serverBusAddress; }
    void initGlobalServerBus(DBusType type, QString busAddress = {}) noexcept
    {
        if (m_initFlag) {
            qCDebug(DDEAMUtils) << "dbus already initialized";
            return;
        }

        m_serverBusAddress = std::move(busAddress);
        m_serverType = type;
        m_initFlag = true;
    }

    QDBusConnection &globalServerBus() noexcept
    {
        if (m_serverConnection.has_value()) {
            return m_serverConnection.value();
        }

        if (!m_initFlag) {
            qFatal("invoke init at first.");
        }

        switch (m_serverType) {
        case DBusType::Session:
            [[fallthrough]];
        case DBusType::System: {
            m_serverConnection.emplace(QDBusConnection::connectToBus(static_cast<QDBusConnection::BusType>(m_serverType),
                                                                     fromStaticRaw(ApplicationManagerServerDBusName)));
            if (!m_serverConnection->isConnected()) {
                qFatal("%s", m_serverConnection->lastError().message().toLocal8Bit().data());
            }
            return m_serverConnection.value();
        }
        case DBusType::Custom: {
            if (m_serverBusAddress.isEmpty()) {
                qFatal("connect to custom dbus must init this object by custom dbus address");
            }
            m_serverConnection.emplace(
                QDBusConnection::connectToBus(m_serverBusAddress, fromStaticRaw(ApplicationManagerServerDBusName)));
            if (!m_serverConnection->isConnected()) {
                qFatal("%s", m_serverConnection->lastError().message().toLocal8Bit().data());
            }
            return m_serverConnection.value();
        }
        }

        Q_UNREACHABLE();
    }
    static ApplicationManager1DBus &instance() noexcept
    {
        static ApplicationManager1DBus dbus;
        return dbus;
    }

    QDBusConnection &globalDestBus() noexcept
    {
        if (!m_destConnection) {
            qFatal("please set which bus should application manager to use to invoke other D-Bus service's method.");
        }
        return m_destConnection.value();
    }

    void setDestBus(QString destAddress = {}) noexcept
    {
        if (m_destConnection) {
            QDBusConnection::disconnectFromBus(fromStaticRaw(ApplicationManagerDestDBusName));
            m_destConnection.reset();
        }

        m_destBusAddress = std::move(destAddress);

        if (m_destBusAddress.isEmpty()) {
            m_destConnection.emplace(QDBusConnection::connectToBus(QDBusConnection::BusType::SessionBus,
                                                                   fromStaticRaw(ApplicationManagerDestDBusName)));
            if (!m_destConnection->isConnected()) {
                qFatal("%s", m_destConnection->lastError().message().toLocal8Bit().data());
            }
            return;
        }

        m_destConnection.emplace(QDBusConnection::connectToBus(m_destBusAddress, fromStaticRaw(ApplicationManagerDestDBusName)));
        if (!m_destConnection->isConnected()) {
            qFatal("%s", m_destConnection->lastError().message().toLocal8Bit().data());
        }
    }

private:
    ApplicationManager1DBus() = default;
    ~ApplicationManager1DBus() = default;
    bool m_initFlag{false};
    DBusType m_serverType{};
    QString m_serverBusAddress;
    QString m_destBusAddress;
    std::optional<QDBusConnection> m_destConnection{std::nullopt};
    std::optional<QDBusConnection> m_serverConnection{std::nullopt};
};

bool registerObjectToDBus(QObject *o, const QString &path, const QString &interface) noexcept;
void unregisterObjectFromDBus(const QString &path) noexcept;

inline const QString &getDBusInterface(const QMetaObject *meta) noexcept
{
    static QHash<const QMetaObject *, QString> interfaceCache{{nullptr, {}}};

    if (Q_UNLIKELY(meta == nullptr)) {
        qCCritical(DDEAMUtils) << "meta is nullptr";
        return interfaceCache[nullptr];  // NOLINT
    }

    auto it = interfaceCache.constFind(meta);
    if (Q_LIKELY(it != interfaceCache.cend())) {
        return *it;
    }

    auto infoIndex = meta->indexOfClassInfo("D-Bus Interface");
    if (Q_LIKELY(infoIndex != -1)) {
        auto interface = QString::fromUtf8(meta->classInfo(infoIndex).value());
        auto it = interfaceCache.insert(meta, std::move(interface));
        return it.value();
    }

    qCWarning(DDEAMUtils) << "no interface found.";
    return interfaceCache[nullptr];  // NOLINT
}

inline ObjectInterfaceMap getChildInterfacesAndPropertiesFromObject(const QObject *o) noexcept
{
    if (Q_UNLIKELY(o == nullptr)) {
        qCCritical(DDEAMUtils) << "object pointer is nullptr";
        return {};
    }

    const auto &childs = o->children();
    ObjectInterfaceMap ret;

    for (const auto *child : childs) {
        const auto *adaptor = qobject_cast<const QDBusAbstractAdaptor *>(child);
        if (adaptor == nullptr) {
            continue;
        }

        const auto *mo = child->metaObject();
        const auto &interface = getDBusInterface(mo);

        if (Q_UNLIKELY(interface.isEmpty())) {
            continue;
        }

        QVariantMap properties;
        for (auto i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
            const auto prop = mo->property(i);
            properties.insert(QString::fromUtf8(prop.name()), prop.read(child));
        }

        ret.insert(interface, properties);
    }

    return ret;
}

inline QStringList getChildInterfacesFromObject(const QObject *o) noexcept
{
    if (Q_UNLIKELY(o == nullptr)) {
        qCCritical(DDEAMUtils) << "object pointer is nullptr";
        return {};
    }

    const auto &childs = o->children();
    QStringList ret;
    ret.reserve(childs.size());

    for (const auto *child : childs) {
        if (const auto *adaptor = qobject_cast<const QDBusAbstractAdaptor *>(child)) {
            auto interface = getDBusInterface(adaptor->metaObject());
            if (!interface.isEmpty()) {
                ret.append(std::move(interface));
            }
        }
    }

    return ret;
}

inline uid_t getCurrentUID()
{
    return getuid();  // current use linux getuid
}

inline QLocale getUserLocale()
{
    return QLocale::system();  // current use env
}

namespace Detail {

inline bool isBaseAlnum(QChar ch) noexcept
{
    const ushort cell = ch.unicode();
    return (cell >= u'a' && cell <= u'z') || (cell >= u'A' && cell <= u'Z') || (cell >= u'0' && cell <= u'9');
}

inline QString unescapeImpl(QStringView str, QStringView prefix) noexcept
{
    const auto prefixLen = prefix.size();
    const auto step = prefixLen + 2;
    const auto len = str.size();

    auto r = str.indexOf(prefix);
    if (r == -1 || r > len - step) {
        return str.toString();
    }

    QString result;
    result.reserve(len);
    result.append(str.first(r));

    while (r < len) {
        if (r <= len - step && str.sliced(r, prefixLen) == prefix) {
            bool ok{false};
            const auto val = str.sliced(r + prefixLen, 2).toShort(&ok, 16);

            if (ok) {
                result.append(QChar::fromLatin1(static_cast<char>(val)));
                r += step;
                continue;
            }
        }

        result.append(str.at(r++));
    }

    if (r < len) {
        result.append(str.sliced(r));
    }

    return result;
}

}  // namespace Detail

inline QString escapeToObjectPath(QStringView str)
{
    using namespace Qt::StringLiterals;
    if (str.isEmpty()) {
        return u"_"_s;
    }

    // see: https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-marshaling-object-path
    auto isSafe = [](QChar ch) { return Detail::isBaseAlnum(ch) || ch == u'_' || ch == u'/'; };

    const auto *it = std::find_if_not(str.cbegin(), str.cend(), isSafe);
    if (it == str.cend()) {
        return str.toString();
    }

    QString result;
    result.reserve(str.size() + 16);
    result.append(str.first(std::distance(str.cbegin(), it)));

    for (; it != str.end(); ++it) {
        const auto ch = *it;
        if (isSafe(ch)) {
            result.append(ch);
        } else {
            if (ch.row() != 0) {
                qCWarning(DDEAMUtils).nospace()
                    << "Character U+" << Qt::hex << ch.unicode() << " is truncated to " << static_cast<uint>(ch.cell());
            }

            result.append(u'_');
            result.append(QString::number(ch.cell(), 16).rightJustified(2, u'0').toLower());
        }
    }

    return result;
}

inline QString unescapeFromObjectPath(QStringView str)
{
    using namespace Qt::StringLiterals;
    return Detail::unescapeImpl(str, u"_"_s);
}

inline QString escapeApplicationId(QStringView id)
{
    if (id.isEmpty()) {
        return id.toString();
    }

    using namespace Qt::StringLiterals;

    // see:
    // https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html#String%20Escaping%20for%20Inclusion%20in%20Unit%20Names
    auto isSafeChar = [](QChar ch) { return Detail::isBaseAlnum(ch) || ch == u'_' || ch == u':' || ch == u'.'; };

    // Custom escape that handles "/" specially
    QString result;
    result.reserve(id.size() + 16);

    for (qsizetype i = 0; i < id.size(); ++i) {
        const auto ch = id.at(i);

        if (ch == u'/') {
            // "/" is replaced by "-"
            result.append(u'-');
        } else if (i == 0 && ch == u'.') {
            // "." as first char is escaped
            result.append(uR"(\x)"_s);
            result.append(QString::number(ch.cell(), 16).rightJustified(2, u'0').toLower());
        } else if (isSafeChar(ch)) {
            result.append(ch);
        } else {
            // Other non-safe chars are escaped
            result.append(uR"(\x)"_s);
            result.append(QString::number(ch.cell(), 16).rightJustified(2, u'0').toLower());
        }
    }

    return result;
}

inline QString unescapeApplicationId(QStringView id)
{
    using namespace Qt::StringLiterals;
    return Detail::unescapeImpl(id, uR"(\x)"_s);
}

inline QString getRelativePathFromAppId(QStringView id)
{
    using namespace Qt::StringLiterals;
    if (id.isEmpty()) {
        return {};
    }

    const auto lastDash = id.lastIndexOf(u'-');
    if (lastDash == -1) {
        return id.toString() % desktopSuffix;
    }

    QString path;
    path.reserve(id.size() + 16);

    qsizetype start{0};
    while (start < lastDash) {
        const auto nextDash = id.indexOf(u'-', start);
        if (nextDash == -1 || nextDash > lastDash) {
            break;
        }

        path.append(u'/');
        path.append(id.sliced(start, nextDash - start));
        start = nextDash + 1;
    }

    path.append(u'-');
    path.append(id.sliced(lastDash + 1));
    path.append(desktopSuffix);

    return path;
}

inline const QString &getDesktopDir() noexcept
{
    static const auto &value{QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)};
    return value;
}

inline const QString &getXDGRuntimeDir() noexcept
{
    static const auto &value{QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)};
    return value;
}

inline const QString &getXDGConfigHome() noexcept
{
    static const auto &value{QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)};
    return value;
}

inline const QString &getXDGDataHome() noexcept
{
    static const auto &value{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    return value;
}

inline const QStringList &getXDGDataDirs() noexcept
{
    static const auto &value{QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)};
    return value;
}

inline const QStringList &getXDGConfigDirs() noexcept
{
    static const auto &value{QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation)};
    return value;
}

inline const QStringList &getAutoStartDirs() noexcept
{
    static const auto &value = []() noexcept {
        auto autostartDirs = getXDGConfigDirs();

        std::for_each(autostartDirs.begin(), autostartDirs.end(), [](QString &str) {
            using namespace Qt::StringLiterals;
            str.append(u"/autostart"_s);
        });

        return autostartDirs;
    }();

    return value;
}

inline const QStringList &getHooksDirs() noexcept
{
    static const auto &value = []() noexcept {
        auto hookDirs = getXDGDataDirs();
        std::for_each(
            hookDirs.begin(), hookDirs.end(), [](QString &str) { str.append(fromStaticRaw(ApplicationManagerHookDir)); });
        return hookDirs;
    }();

    return value;
}

inline const QString &getUserApplicationDir() noexcept
{
    static const auto &value{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    return value;
}

inline const QStringList &getApplicationsDirs() noexcept
{
    static const auto &value{QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation)};
    return value;
}

inline const QStringList &getMimeDirs() noexcept
{
    static const auto &value = []() noexcept {
        auto mimeDirs = getXDGConfigDirs();
        mimeDirs.append(getApplicationsDirs());
        return mimeDirs;
    }();

    return value;
}

inline const QStringList &getCurrentDesktops()
{
    using namespace Qt::StringLiterals;
    static const auto &desktops = [] {
        auto value = qEnvironmentVariable("XDG_CURRENT_DESKTOP", u"dde"_s);
        auto tokens = qTokenize(value, u':');

        QStringList list;
        for (auto token : tokens) {
            list.append(token.toString().toLower());
        }

        list.removeDuplicates();
        return list;
    }();

    return desktops;
}

struct UnitInfo
{
    QString applicationID;
    QString launcher;
    QString instanceID;
};

[[nodiscard]] inline std::optional<UnitInfo> processUnitName(QStringView unitName) noexcept
{
    using namespace Qt::StringLiterals;
    qCDebug(DDEAMUtils) << "process unit:" << unitName;

    constexpr QStringView appPrefix{u"app-"};
    if (!unitName.startsWith(appPrefix)) {
        // If not started by application manager, just remove suffix and take name as app id.
        auto lastDotIndex = unitName.lastIndexOf(u'.');
        if (lastDotIndex == -1) {
            return std::nullopt;
        }

        return std::make_optional<UnitInfo>({unescapeApplicationId(unitName.first(lastDotIndex)), {}, {}});
    }

    auto unit = unitName.sliced(appPrefix.size());
    auto current = unit;
    auto popSuffix = [&](char16_t sep) -> QStringView {
        if (current.isEmpty()) {
            return {};
        }

        const auto idx = current.lastIndexOf(sep);
        if (idx == -1) {
            return std::exchange(current, {});
        }

        const auto res = current.sliced(idx + 1);
        current = current.first(idx);
        return res;
    };

    if (unit.endsWith(u".service")) {
        current = unit.first(unit.lastIndexOf(u'.'));

        const auto vInstance = popSuffix(u'@');
        const auto vApp = popSuffix(u'-');
        const auto vLauncher = current;

        return UnitInfo{unescapeApplicationId(vApp), vLauncher.toString(), vInstance.toString()};
    }

    if (unit.endsWith(u".scope")) {
        current = unit.first(unit.lastIndexOf(u'.'));

        const auto vInstance = popSuffix(u'-');
        const auto vApp = popSuffix(u'-');
        const auto vLauncher = current;

        return UnitInfo{unescapeApplicationId(vApp), vLauncher.toString(), vInstance.toString()};
    }

    qCDebug(DDEAMUtils) << "it's not service or scope:" << unit << ",ignore";
    return std::nullopt;
}

template <typename Key, typename Value>
ObjectMap dumpDBusObject(const QHash<Key, QSharedPointer<Value>> &map)
{
    static_assert(std::is_base_of_v<QObject, Value>, "dumpDBusObject only support which derived by QObject class");
    static_assert(std::is_same_v<Key, QString> || std::is_same_v<Key, QDBusObjectPath>,
                  "dumpDBusObject only support QString/QDBusObject as key type");
    ObjectMap objs;

    for (const auto &[key, value] : map.asKeyValueRange()) {
        auto interAndProps = getChildInterfacesAndPropertiesFromObject(value.data());
        if (interAndProps.isEmpty()) {
            continue;
        }

        if constexpr (std::is_same_v<Key, QString>) {
            objs.insert(QDBusObjectPath{getObjectPathFromAppId(key)}, std::move(interAndProps));
        } else if constexpr (std::is_same_v<Key, QDBusObjectPath>) {
            objs.insert(key, std::move(interAndProps));
        }
    }

    return objs;
}

inline QByteArray getCurrentSessionId()
{
    using namespace Qt::StringLiterals;

    auto msg = QDBusMessage::createMethodCall(QString::fromUtf8(SystemdService),
                                              QString::fromUtf8(SystemdObjectPath) % u"/unit/"_s %
                                                  escapeToObjectPath(u"graphical-session.target"),
                                              fromStaticRaw(SystemdPropInterfaceName),
                                              u"Get"_s);
    msg << fromStaticRaw(SystemdUnitInterfaceName);
    msg << u"InvocationID"_s;

    auto ret = QDBusConnection::sessionBus().call(msg, QDBus::Block, DBusStartupCallTimeoutMs);
    if (ret.type() != QDBusMessage::ReplyMessage) {
        qCWarning(DDEAMUtils) << "get graphical session Id failed:" << ret.errorMessage();
        return {};
    }

    const auto &args = ret.arguments();
    if (args.isEmpty()) {
        return {};
    }

    const auto &id = args.constFirst();
    return id.value<QDBusVariant>().variant().toByteArray();
}

inline uint getPidFromPidFd(const QDBusUnixFileDescriptor &pidfd) noexcept
{
    using namespace Qt::StringLiterals;
    auto fdFilePath = u"/proc/self/fdinfo/"_s % QString::number(pidfd.fileDescriptor());
    QFile fdFile{fdFilePath};
    if (!fdFile.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text)) {
        qCWarning(DDEAMUtils) << "Failed to open" << fdFilePath << fdFile.errorString();
        return 0;
    }

    while (true) {
        const auto line = fdFile.readLine();
        if (line.isEmpty()) {
            break;
        }

        if (line.startsWith("Pid:"_ba)) {
            const auto pidView = QByteArrayView(line).sliced(4).trimmed();

            bool ok{false};
            const auto pid = pidView.toUInt(&ok);

            if (ok) {
                return pid;
            }

            break;
        }
    }

    qCWarning(DDEAMUtils) << "Could not find valid 'Pid' field in" << fdFilePath;
    return 0;
}

inline QString getAutostartAppIdFromAbsolutePath(QStringView path)
{
    using namespace Qt::StringLiterals;
    if (!path.endsWith(desktopSuffix)) {
        return {};
    }
    auto base = path.chopped(desktopSuffix.size());
    auto tokens = qTokenize(base, QDir::separator());

    QString result;
    bool foundAutostart{false};
    for (auto component : tokens) {
        if (!foundAutostart) {
            if (component == u"autostart") {
                foundAutostart = true;
            }
            continue;
        }

        if (component.isEmpty()) {
            continue;
        }

        if (!result.isEmpty()) {
            result.append(u'-');
        }

        result.append(component);
    }

    return result;
}

inline QString getObjectPathFromAppId(const QString &appId)
{
    const auto &basePath = fromStaticRaw(DDEApplicationManager1ObjectPath);
    if (!appId.isEmpty()) {
        return basePath % '/' % escapeToObjectPath(appId);
    }

    return basePath % '/' % QUuid::createUuid().toString(QUuid::Id128);
}

inline int pidfd_open(pid_t pid, uint flags) noexcept
{
    return syscall(SYS_pidfd_open, pid, flags);
}

inline int pidfd_send_signal(int pidfd, int sig, siginfo_t *info, unsigned int flags) noexcept
{
    return syscall(SYS_pidfd_send_signal, pidfd, sig, info, flags);
}

#define safe_sendErrorReply                                                                                                      \
    if (calledFromDBus())                                                                                                        \
    sendErrorReply

#endif
