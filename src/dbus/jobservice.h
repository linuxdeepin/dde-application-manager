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
    ~JobService() override;

    Q_PROPERTY(QString Status READ status)
    QString status() const;

public Q_SLOTS:
    void Cancel();
    void Pause();
    void Resume();

private:
    explicit JobService(const QFuture<QVariant>& job);
    QFuture<QVariant> m_job;
};

#endif
