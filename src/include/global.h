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

using IconMap = QMap<QString, QMap<uint, QMap<QString, QDBusUnixFileDescriptor>>>;

constexpr auto DDEApplicationManager1ServiceName = u8"org.deepin.dde.ApplicationManager1";
constexpr auto DDEApplicationManager1ObjectPath = u8"/org/deepin/dde/ApplicationManager1";
constexpr auto DDEApplicationManager1ApplicationObjectPath = u8"/org/deepin/dde/ApplicationManager1/Application/";
constexpr auto DDEApplicationManager1JobManagerObjectPath = u8"/org/deepin/dde/ApplicationManager1/JobManager1";
constexpr auto DDEApplicationManager1JobObjectPath = u8"/org/deepin/dde/ApplicationManager1/JobManager1/Job/";

class ApplicationManager1DBus
{
public:
    ApplicationManager1DBus(const ApplicationManager1DBus &) = delete;
    ApplicationManager1DBus(ApplicationManager1DBus &&) = delete;
    ApplicationManager1DBus &operator=(const ApplicationManager1DBus &) = delete;
    ApplicationManager1DBus &operator=(ApplicationManager1DBus&&) =  delete;
    const QString &BusAddress() { return m_busAddress; }
    void setBusAddress(const QString &address) { m_busAddress = address; }
    QDBusConnection& CustomBus()
    {
        static auto con = QDBusConnection::connectToBus(m_busAddress, "org.deepin.dde.ApplicationManager1");
        if (!con.isConnected()) {
            qWarning() << con.lastError();
            std::terminate();
        }
        return con;
    }
    static ApplicationManager1DBus &instance()
    {
        static ApplicationManager1DBus dbus;
        return dbus;
    }

private:
    ApplicationManager1DBus() = default;
    ~ApplicationManager1DBus() = default;
    QString m_busAddress;
};

template <typename T, typename U>
bool registerObjectToDbus(T parent, const QString &path, const QString &interface)
{
    using service_type = std::remove_const_t<T>;
    static_assert(std::is_pointer_v<service_type>, "param type must be a pointer");
    static_assert(std::is_base_of_v<QObject, std::remove_pointer_t<T>> and
                      std::is_base_of_v<QObject, U>,
                  "param type must derive QObject");
    auto &con = ApplicationManager1DBus::instance().CustomBus();
    if (!con.registerObject(path, interface, new U{parent})) {
        qWarning() << "register object failed:" << path << interface << con.lastError();
        return false;
    }
    return true;
}

template <typename T>
QString getDBusInterface()
{
    auto meta = QMetaType::fromType<T>();
    auto infoObject = meta.metaObject();
    if (auto infoIndex = infoObject->indexOfClassInfo("D-Bus Interface"); infoIndex != -1) {
        return infoObject->classInfo(infoIndex).value();
    }
    return {};
}

#endif
