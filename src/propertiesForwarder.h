// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PROPERTIESFORWARDER_H
#define PROPERTIESFORWARDER_H

#include <QObject>

class PropertiesForwarder : public QObject
{
    Q_OBJECT
public:
    explicit PropertiesForwarder(QString path, QString interfaceName, QObject *parent);
public Q_SLOTS:
    void PropertyChanged();

private:
    QString m_path;
    QString m_interfaceName;
};

#endif
