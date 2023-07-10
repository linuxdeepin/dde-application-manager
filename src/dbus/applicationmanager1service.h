// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONMANAGER1SERVICE_H
#define APPLICATIONMANAGER1SERVICE_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>

class ApplicationManager1Service : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationManager1Service(QObject *parent = nullptr);
    virtual ~ApplicationManager1Service();

public:
    Q_PROPERTY(QList<QDBusObjectPath> List READ list)
    QList<QDBusObjectPath> list() const;

public Q_SLOTS:
    QDBusObjectPath Application(const QString &id);
    QString Identify(const QDBusUnixFileDescriptor &pidfd, QDBusObjectPath &application, QDBusObjectPath &application_instance);
    QDBusObjectPath Launch(const QString &id, const QString &action, const QStringList &fields, const QVariantMap &options);
};

#endif
