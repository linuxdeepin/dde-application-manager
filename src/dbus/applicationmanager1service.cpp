// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "applicationmanager1service.h"
#include "applicationadaptor.h"
#include "global.h"

ApplicationManager1Service::~ApplicationManager1Service() = default;

ApplicationManager1Service::ApplicationManager1Service() = default;

QList<QDBusObjectPath> ApplicationManager1Service::list() const { return m_applicationList.keys(); }

bool ApplicationManager1Service::removeOneApplication(const QDBusObjectPath &application)
{
    return m_applicationList.remove(application) != 0;
}

void ApplicationManager1Service::removeAllApplication() { m_applicationList.clear(); }

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
                                                   const QString &actions,
                                                   const QStringList &fields,
                                                   const QVariantMap &options)
{
    // TODO: impl reset of Launch
    QString objectPath;
    if (id.contains('.')) {
        objectPath = id.split('.').first();
    }
    objectPath.prepend(DDEApplicationManager1ApplicationObjectPath);
    QSharedPointer<ApplicationService> app{new ApplicationService{id}};
    auto *ptr = app.data();
    if (registerObjectToDbus(new ApplicationAdaptor(ptr), objectPath, getDBusInterface<ApplicationAdaptor>())) {
        QDBusObjectPath path{objectPath};
        m_applicationList.insert(path, app);
        return path;
    }
    return {};
}
