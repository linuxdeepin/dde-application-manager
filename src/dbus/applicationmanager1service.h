// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONMANAGER1SERVICE_H
#define APPLICATIONMANAGER1SERVICE_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QSharedPointer>
#include <QMap>
#include "applicationservice.h"

class ApplicationManager1Service final : public QObject
{
    Q_OBJECT
public:
    ApplicationManager1Service();
    ~ApplicationManager1Service() override;
    ApplicationManager1Service(const ApplicationManager1Service &) = delete;
    ApplicationManager1Service(ApplicationManager1Service &&) = delete;
    ApplicationManager1Service &operator=(const ApplicationManager1Service &) = delete;
    ApplicationManager1Service &operator=(ApplicationManager1Service &&) = delete;

    Q_PROPERTY(QList<QDBusObjectPath> List READ list)
    QList<QDBusObjectPath> list() const;
    void addApplication(const QString &ID, const QStringList &actions, bool AutoStart = false);
    bool removeOneApplication(const QDBusObjectPath &application);
    void removeAllApplication();

public Q_SLOTS:
    QDBusObjectPath Application(const QString &id);
    QString Identify(const QDBusUnixFileDescriptor &pidfd, QDBusObjectPath &application, QDBusObjectPath &application_instance);
    QDBusObjectPath Launch(const QString &id, const QString &action, const QStringList &fields, const QVariantMap &options);

private:
    QMap<QDBusObjectPath, QSharedPointer<ApplicationService>> m_applicationList;
};

#endif
