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
#include <QDir>
#include <QRegularExpression>
#include <QDBusObjectPath>
#include <unistd.h>
#include "constant.h"
#include "config.h"

using IconMap = QMap<QString, QMap<uint, QMap<QString, QDBusUnixFileDescriptor>>>;

inline QString getApplicationLauncherBinary()
{
    static const QString bin = []() {
        auto value = qgetenv("DEEPIN_APPLICATION_MANAGER_APP_LAUNCH_HELPER_BIN");
        if (value.isEmpty()) {
            return QString::fromLocal8Bit(ApplicationLaunchHelperBinary);
        }
        qWarning() << "Using app launch helper defined in environment variable DEEPIN_APPLICATION_MANAGER_APP_LAUNCH_HELPER_BIN.";
        return QString::fromLocal8Bit(value);
    }();
    return bin;
}

enum class DBusType { Session = QDBusConnection::SessionBus, System = QDBusConnection::SystemBus, Custom };

template <typename T>
using remove_cvr_t = std::remove_reference_t<std::remove_cv_t<T>>;

template <typename T>
void applyIteratively(QList<QDir> &&dirs, T &&func)
{
    static_assert(std::is_invocable_v<T, const QFileInfo &>, "apply function should only accept one QFileInfo");
    static_assert(std::is_same_v<decltype(func(QFileInfo{})), bool>,
                  "apply function should return a boolean to indicate when should return");
    QList<QDir> dirList{std::move(dirs)};

    while (!dirList.isEmpty()) {
        const auto dir = dirList.takeFirst();

        if (!dir.exists()) {
            qWarning() << "apply function to an non-existent directory:" << dir.absolutePath() << ", skip.";
            continue;
        }

        const auto &infoList =
            dir.entryInfoList({"*.desktop"},
                              QDir::Readable | QDir::AllDirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot,
                              QDir::Name | QDir::DirsLast);

        for (const auto &info : infoList) {
            if (info.isFile() and func(info)) {
                return;
            }

            if (info.isDir()) {
                dirList.append(info.absoluteFilePath());
            }
        }
    }
}

class ApplicationManager1DBus
{
public:
    ApplicationManager1DBus(const ApplicationManager1DBus &) = delete;
    ApplicationManager1DBus(ApplicationManager1DBus &&) = delete;
    ApplicationManager1DBus &operator=(const ApplicationManager1DBus &) = delete;
    ApplicationManager1DBus &operator=(ApplicationManager1DBus &&) = delete;
    [[nodiscard]] const QString &globalDestBusAddress() const { return m_destBusAddress; }
    [[nodiscard]] const QString &globalServerBusAddress() const { return m_serverBusAddress; }
    void initGlobalServerBus(DBusType type, const QString &busAddress = "")
    {
        if (m_initFlag) {
            return;
        }

        m_serverBusAddress = busAddress;
        m_serverType = type;
        m_initFlag = true;
    }

    QDBusConnection &globalServerBus()
    {
        if (m_serverConnection.has_value()) {
            return m_serverConnection.value();
        }

        if (!m_initFlag) {
            qFatal() << "invoke init at first.";
        }

        switch (m_serverType) {
            case DBusType::Session:
                [[fallthrough]];
            case DBusType::System: {
                m_serverConnection.emplace(QDBusConnection::connectToBus(static_cast<QDBusConnection::BusType>(m_serverType),
                                                                         ApplicationManagerServerDBusName));
                if (!m_serverConnection->isConnected()) {
                    qFatal() << m_serverConnection->lastError();
                }
                return m_serverConnection.value();
            }
            case DBusType::Custom: {
                if (m_serverBusAddress.isEmpty()) {
                    qFatal() << "connect to custom dbus must init this object by custom dbus address";
                }
                m_serverConnection.emplace(QDBusConnection::connectToBus(m_serverBusAddress, ApplicationManagerServerDBusName));
                if (!m_serverConnection->isConnected()) {
                    qFatal() << m_serverConnection->lastError();
                }
                return m_serverConnection.value();
            }
        }

        Q_UNREACHABLE();
    }
    static ApplicationManager1DBus &instance()
    {
        static ApplicationManager1DBus dbus;
        return dbus;
    }

    QDBusConnection &globalDestBus()
    {
        if (!m_destConnection) {
            qFatal() << "please set which bus should application manager to use to invoke other D-Bus service's method.";
        }
        return m_destConnection.value();
    }

    void setDestBus(const QString &destAddress)
    {
        if (m_destConnection) {
            m_destConnection->disconnectFromBus(ApplicationManagerDestDBusName);
        }

        m_destBusAddress = destAddress;

        if (m_destBusAddress.isEmpty()) {
            m_destConnection.emplace(
                QDBusConnection::connectToBus(QDBusConnection::BusType::SessionBus, ApplicationManagerDestDBusName));
            if (!m_destConnection->isConnected()) {
                qFatal() << m_destConnection->lastError();
            }
            return;
        }

        if (m_destBusAddress.isEmpty()) {
            qFatal() << "connect to custom dbus must init this object by custom dbus address";
        }

        m_destConnection.emplace(QDBusConnection::connectToBus(m_destBusAddress, ApplicationManagerDestDBusName));
        if (!m_destConnection->isConnected()) {
            qFatal() << m_destConnection->lastError();
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
            ret.replace(QString{"_%1"}.arg(hexStr), QChar::fromLatin1(hexStr.toUInt(nullptr, 16)));
            i += 2;
        }
    }
    return ret;
}

inline QString escapeApplicationId(const QString &id)
{
    if (id.isEmpty()) {
        return id;
    }

    auto ret = id;
    QRegularExpression re{R"([^a-zA-Z0-9])"};
    auto matcher = re.globalMatch(ret);
    while (matcher.hasNext()) {
        auto replaceList = matcher.next().capturedTexts();
        replaceList.removeDuplicates();
        for (const auto &c : replaceList) {
            auto hexStr = QString::number(static_cast<uint>(c.front().toLatin1()), 16);
            ret.replace(c, QString{R"(\x%1)"}.arg(hexStr));
        }
    }
    return ret;
}

inline QString unescapeApplicationId(const QString &id)
{
    auto ret = id;
    for (qsizetype i = 0; i < id.size(); ++i) {
        if (id[i] == '\\' and i + 3 < id.size()) {
            auto hexStr = id.sliced(i + 2, 2);
            ret.replace(QString{R"(\x%1)"}.arg(hexStr), QChar::fromLatin1(hexStr.toUInt(nullptr, 16)));
            i += 3;
        }
    }
    return ret;
}

inline QString getRelativePathFromAppId(const QString &id)
{
    QString path;
    auto components = id.split('-', Qt::SkipEmptyParts);
    for (qsizetype i = 0; i < components.size() - 1; ++i) {
        path += QString{"/%1"}.arg(components[i]);
    }
    path += QString{R"(-%1.desktop)"}.arg(components.last());
    return path;
}

inline QStringList getXDGDataDirs()
{
    const static auto XDGDataDirs = []() {
        auto XDGDataDirs = QString::fromLocal8Bit(qgetenv("XDG_DATA_DIRS")).split(':', Qt::SkipEmptyParts);

        if (XDGDataDirs.isEmpty()) {
            XDGDataDirs.append("/usr/local/share");
            XDGDataDirs.append("/usr/share");
        }

        auto XDGDataHome = QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME"));
        if (XDGDataHome.isEmpty()) {
            XDGDataHome = QString{qgetenv("HOME")} + QDir::separator() + ".local" + QDir::separator() + "share";
        }

        XDGDataDirs.push_front(XDGDataHome);

        std::for_each(XDGDataDirs.begin(), XDGDataDirs.end(), [](QString &str) {
            if (!str.endsWith(QDir::separator())) {
                str.append(QDir::separator());
            }
            str.append("applications");
        });

        return XDGDataDirs;
    }();

    return XDGDataDirs;
}

#endif
