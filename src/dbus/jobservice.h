// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef JOBSERVICE_H
#define JOBSERVICE_H

#include <QObject>
#include <QFuture>
#include <QVariant>

class JobService : public QObject
{
    Q_OBJECT
public:
    explicit JobService(QObject *parent = nullptr);
    virtual ~JobService();

public:
    Q_PROPERTY(QString Status READ status)
    QString status() const;

public Q_SLOTS:
    void Cancel();
    void Pause();
    void Resume();

private:
    QFuture<QVariant> job;
};

#endif
