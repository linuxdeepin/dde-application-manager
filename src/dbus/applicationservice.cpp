// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationservice.h"

ApplicationService::ApplicationService(QObject *parent)
    : QObject(parent)
{
}

ApplicationService::~ApplicationService() {}

QDBusObjectPath ApplicationService::Launch(const QString &action, const QStringList &fields, const QVariantMap &options)
{
    // TODO: impl
    return {};
}
