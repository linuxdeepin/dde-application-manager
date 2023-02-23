// SPDX-FileCopyrightText: 2015 Jolla Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//


#ifndef DBUSEXTENDEDPENDINGCALLWATCHER_P_H
#define DBUSEXTENDEDPENDINGCALLWATCHER_P_H

#include <QDBusPendingCallWatcher>
#include <QDBusError>

class DBusExtendedPendingCallWatcher: public QDBusPendingCallWatcher
{
    Q_OBJECT

public:
    explicit DBusExtendedPendingCallWatcher(const QDBusPendingCall &call,
                                            const QString &asyncProperty,
                                            const QVariant &previousValue,
                                            QObject *parent = 0);
    ~DBusExtendedPendingCallWatcher();

    Q_PROPERTY(QString AsyncProperty READ asyncProperty)
    inline QString asyncProperty() const { return m_asyncProperty; }

    Q_PROPERTY(QVariant PreviousValue READ previousValue)
    inline QVariant previousValue() const { return m_previousValue; }

private:
    QString m_asyncProperty;
    QVariant m_previousValue;
};

#endif /* DBUSEXTENDEDPENDINGCALLWATCHER_P_H */
