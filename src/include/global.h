// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QMap>
#include <QDBusUnixFileDescriptor>
#include <QDBusConnection>
#include <optional>
#include <QDBusError>
#include <QMetaType>
#include <QMetaClassInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QDBusObjectPath>
#include <unistd.h>
#include <optional>

using IconMap = QMap<QString, QMap<uint, QMap<QString, QDBusUnixFileDescriptor>>>;

constexpr auto DDEApplicationManager1ServiceName = u8"org.deepin.dde.ApplicationManager1";
constexpr auto DDEApplicationManager1ObjectPath = u8"/org/deepin/dde/ApplicationManager1";
constexpr auto DDEApplicationManager1ApplicationObjectPath = u8"/org/deepin/dde/ApplicationManager1/Application/";
constexpr auto DDEApplicationManager1InstanceObjectPath = u8"/org/deepin/dde/ApplicationManager1/Instance/";
constexpr auto DDEApplicationManager1JobManagerObjectPath = u8"/org/deepin/dde/ApplicationManager1/JobManager1";
constexpr auto DDEApplicationManager1JobObjectPath = u8"/org/deepin/dde/ApplicationManager1/JobManager1/Job/";
constexpr auto DesktopFileEntryKey = u8"Desktop Entry";
constexpr auto DesktopFileActionKey = u8"Desktop Action ";
constexpr auto ApplicationLauncherBinary =
    u8"/home/heyuming/workspace/dde-application-manager/build/plugin/appLauncher/am-launcher";
constexpr auto ApplicationManagerDBusName = u8"deepin_application_manager_bus";

enum class DBusType { Session = QDBusConnection::SessionBus, System = QDBusConnection::SystemBus, Custom };

class ApplicationManager1DBus
{
public:
    ApplicationManager1DBus(const ApplicationManager1DBus &) = delete;
    ApplicationManager1DBus(ApplicationManager1DBus &&) = delete;
    ApplicationManager1DBus &operator=(const ApplicationManager1DBus &) = delete;
    ApplicationManager1DBus &operator=(ApplicationManager1DBus &&) = delete;
    const QString &BusAddress() { return m_busAddress; }
    void init(DBusType type, const QString &busAddress = "")
    {
        if (m_initFlag) {
            return;
        }

        m_busAddress = busAddress;
        m_type = type;
        m_initFlag = true;
        return;
    }

    QDBusConnection &globalBus()
    {
        if (m_connection.has_value()) {
            return m_connection.value();
        }

        if (!m_initFlag) {
            qFatal() << "invoke init at first.";
        }

        switch (m_type) {
            case DBusType::Session:
                [[fallthrough]];
            case DBusType::System: {
                m_connection.emplace(
                    QDBusConnection::connectToBus(static_cast<QDBusConnection::BusType>(m_type), ApplicationManagerDBusName));
                if (!m_connection->isConnected()) {
                    qFatal() << m_connection->lastError();
                }
                return m_connection.value();
            }
            case DBusType::Custom: {
                if (m_busAddress.isEmpty()) {
                    qFatal() << "connect to custom dbus must init this object by custom dbus address";
                }
                m_connection.emplace(
                    QDBusConnection::connectToBus(static_cast<QDBusConnection::BusType>(m_type), ApplicationManagerDBusName));
                if (!m_connection->isConnected()) {
                    qFatal() << m_connection->lastError();
                }
                return m_connection.value();
            }
        }
        Q_UNREACHABLE();
    }
    static ApplicationManager1DBus &instance()
    {
        static ApplicationManager1DBus dbus;
        return dbus;
    }

private:
    ApplicationManager1DBus() = default;
    ~ApplicationManager1DBus() = default;
    bool m_initFlag;
    DBusType m_type;
    QString m_busAddress;
    std::optional<QDBusConnection> m_connection{std::nullopt};
};

bool registerObjectToDBus(QObject *o, const QString &path, const QString &interface);
void unregisterObjectFromDBus(const QString &path);

template <typename T>
QString getDBusInterface()
{
    auto meta = QMetaType::fromType<T>();
    auto infoObject = meta.metaObject();
    if (auto infoIndex = infoObject->indexOfClassInfo("D-Bus Interface"); infoIndex != -1) {
        return infoObject->classInfo(infoIndex).value();
    }
    qWarning() << "no interface found.";
    return {};
}

inline uid_t getCurrentUID()
{
    return getuid();  // current use linux getuid
}

inline QLocale getUserLocale()
{
    return QLocale::system();  // current use env
}

inline QString unescapeString(const QString &input)
{
    QRegularExpression regex("\\\\x([0-9A-Fa-f]{2})");
    QRegularExpressionMatchIterator it = regex.globalMatch(input);
    QString output{input};

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        bool ok;
        // Get the hexadecimal value from the match and convert it to a number.
        int asciiValue = match.captured(1).toInt(&ok, 16);
        if (ok) {
            // Convert the ASCII value to a QChar and perform the replacement.
            output.replace(match.capturedStart(), match.capturedLength(), QChar(asciiValue));
        }
    }

    return output;
}

inline QString escapeToObjectPath(const QString &str)
{
    if (str.isEmpty()) {
        return "_";
    }

    auto ret = str;
    QRegularExpression re{R"([^a-zA-Z0-9])"};
    auto matcher = re.globalMatch(ret);
    while (matcher.hasNext()) {
        auto replaceList = matcher.next().capturedTexts();
        replaceList.removeDuplicates();
        for (const auto &c : replaceList) {
            auto hexStr = QString::number(static_cast<uint>(c.front().toLatin1()), 16);
            ret.replace(c, QString{R"(_%1)"}.arg(hexStr));
        }
    }
    return ret;
}

inline QString unescapeFromObjectPath(const QString &str)
{
    auto ret = str;
    for (qsizetype i = 0; i < str.size(); ++i) {
        if (str[i] == '_' and i + 2 < str.size()) {
            auto hexStr = str.sliced(i + 1, 2);
            ret.replace(hexStr, QChar::fromLatin1(hexStr.toUInt(nullptr, 16)));
            i += 2;
        }
    }
    return ret;
}

#endif
