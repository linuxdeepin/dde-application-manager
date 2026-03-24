// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"

bool registerObjectToDBus(QObject *o, const QString &path, const QString &interface) noexcept
{
    if (o == nullptr) {
        qCCritical(DDEAMUtils) << "Attempted to register a null object to" << path;
        return false;
    }

    auto &con = ApplicationManager1DBus::instance().globalServerBus();
    if (!con.registerObject(path, interface, o, QDBusConnection::RegisterOption::ExportAdaptors)) {
        qCCritical(DDEAMUtils) << "Register object failed!"
                               << "Path:" << path << "Interface:" << interface << "Error:" << con.lastError().message();
        return false;
    }

    qCDebug(DDEAMUtils) << "Object registered successfully at" << path << "with interface" << interface;
    return true;
}

void unregisterObjectFromDBus(const QString &path) noexcept
{
    auto &con = ApplicationManager1DBus::instance().globalServerBus();
    con.unregisterObject(path);
    qCDebug(DDEAMUtils) << "Object unregistered from" << path;
}
