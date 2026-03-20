// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <DExpected>
#include <QString>

class Launcher
{
public:
    enum class Type : uint8_t {
        Unknown,
        ByUser,
    };
    void setPath(const QString &path);
    void setAction(const QString &action);
    void setLaunchedType(Type type);
    void setAutostart(bool autostart);
    void setEnvironmentVariables(const QStringList &envVars);
    Dtk::Core::DExpected<void> run();

    static Dtk::Core::DExpected<QStringList> appIds();

private:
    Dtk::Core::DExpected<void> launch() noexcept;
    Dtk::Core::DExpected<void> updateLaunchedTimes() noexcept;
    [[nodiscard]] QString appId() const noexcept;

    QString m_path;
    QString m_action;
    Type m_launchedType{Type::Unknown};
    bool m_autostart = false;
    QStringList m_environmentVariables;
};
