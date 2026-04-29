// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SYSTEMDSIGNALDISPATCHER_H
#define SYSTEMDSIGNALDISPATCHER_H

#include "global.h"

class SystemdSignalDispatcher : public QObject
{
    Q_OBJECT
public:
    ~SystemdSignalDispatcher() override = default;
    static SystemdSignalDispatcher &instance()
    {
        static SystemdSignalDispatcher dispatcher;
        return dispatcher;
    }
    [[nodiscard]] bool isAvailable() const noexcept { return m_available; }
Q_SIGNALS:
    void SystemdUnitNew(const QString &unitName, const QDBusObjectPath &systemdUnitPath);
    void SystemdJobNew(const QString &unitName, const QDBusObjectPath &systemdUnitPath);
    void SystemdUnitRemoved(const QString &unitName, const QDBusObjectPath &systemdUnitPath);
    void SystemdEnvironmentChanged(const QStringList &envs);

private Q_SLOTS:
    void onUnitNew(const QString &unitName, const QDBusObjectPath &systemdUnitPath);
    void onJobNew(uint32_t id, const QDBusObjectPath &systemdUnitPath, const QString &unitName);
    void onUnitRemoved(const QString &unitName, const QDBusObjectPath &systemdUnitPath);
    void onPropertiesChanged(const QString &interface, const QVariantMap &props, const QStringList &invalid);

private:
    bool m_available{false};

    explicit SystemdSignalDispatcher(QObject *parent = nullptr)
        : QObject(parent)
    {
        using namespace Qt::StringLiterals;
        auto ret = ApplicationManager1DBus::instance().globalDestBus().call(
            QDBusMessage::createMethodCall(SystemdService, SystemdObjectPath, SystemdInterfaceName, u"Subscribe"_s),
            QDBus::Block,
            DBusStartupCallTimeoutMs);
        if (ret.type() == QDBusMessage::ErrorMessage) {
            qWarning() << "can't subscribe to systemd user manager:" << ret.errorMessage();
            return;
        }

        if (!connectToSignals()) {
            qWarning() << "can't connect to systemd user manager signals.";
            return;
        }

        m_available = true;
    }

    bool connectToSignals() noexcept;
};

#endif
