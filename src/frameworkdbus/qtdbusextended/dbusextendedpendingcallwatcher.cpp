// SPDX-FileCopyrightText: 2015 Jolla Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusextendedpendingcallwatcher_p.h"


DBusExtendedPendingCallWatcher::DBusExtendedPendingCallWatcher(const QDBusPendingCall &call, const QString &asyncProperty, const QVariant &previousValue, QObject *parent)
    : QDBusPendingCallWatcher(call, parent)
    , m_asyncProperty(asyncProperty)
    , m_previousValue(previousValue)
{
}

DBusExtendedPendingCallWatcher::~DBusExtendedPendingCallWatcher()
{
}
