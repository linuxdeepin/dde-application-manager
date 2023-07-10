// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationmanager1service.h"

ApplicationManager1Service::ApplicationManager1Service(QObject *parent)
    : QObject(parent)
{
}

ApplicationManager1Service::~ApplicationManager1Service() {}

QList<QDBusObjectPath> ApplicationManager1Service::list() const
{
    // TODO: impl
    return {};
}

QDBusObjectPath ApplicationManager1Service::Application(const QString &id)
{
    // TODO: impl
    return {};
}

QString ApplicationManager1Service::Identify(const QDBusUnixFileDescriptor &pidfd,
                                             QDBusObjectPath &application,
                                             QDBusObjectPath &application_instance)
{
    // TODO: impl
    return {};
}

QDBusObjectPath ApplicationManager1Service::Launch(const QString &id,
                                                   const QString &action,
                                                   const QStringList &fields,
                                                   const QVariantMap &options)
{
    // TODO: impl
    return {};
}
