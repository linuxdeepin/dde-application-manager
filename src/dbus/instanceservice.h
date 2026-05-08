// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef INSTANCESERVICE_H
#define INSTANCESERVICE_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusContext>

class InstanceService : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    ~InstanceService() override;
    InstanceService(const InstanceService &) = delete;
    InstanceService(InstanceService &&) = delete;
    InstanceService &operator=(const InstanceService &) = delete;
    InstanceService &operator=(InstanceService &&) = delete;

    Q_PROPERTY(QDBusObjectPath Application MEMBER m_Application CONSTANT)
    Q_PROPERTY(QDBusObjectPath SystemdUnitPath MEMBER m_SystemdUnitPath CONSTANT)
    Q_PROPERTY(QString Launcher MEMBER m_Launcher CONSTANT)
    Q_PROPERTY(QString LaunchType MEMBER m_launchType CONSTANT)
    Q_PROPERTY(bool Orphaned MEMBER m_orphaned NOTIFY orphanedChanged)

    [[nodiscard]] const QString &instanceId() const noexcept { return m_instanceId; }
    [[nodiscard]] const QString &launchType() const noexcept { return m_launchType; }
    [[nodiscard]] const QString &launchUniqueId() const noexcept { return m_uniqueId; }

public Q_SLOTS:
    void KillAll(int signal);

Q_SIGNALS:
    void orphanedChanged();

private:
    friend class ApplicationService;
    InstanceService(QString instanceId, QString application, QString systemdUnitPath, QString launcher, const QString &launchType = {}, const QString &uniqueId = {});
    bool m_orphaned{false};
    QString m_Launcher;
    QString m_launchType;
    QString m_uniqueId;
    QString m_instanceId;
    QDBusObjectPath m_Application;
    QDBusObjectPath m_SystemdUnitPath;
};

static inline QList<QSharedPointer<InstanceService>> orphanedInstances{};

#endif
