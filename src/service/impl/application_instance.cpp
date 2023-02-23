// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application_instance.h"
#include "../applicationhelper.h"
#include "application.h"
#include "instanceadaptor.h"

#include <qdatetime.h>
#include <QCryptographicHash>
#include <QDateTime>
#include <QProcess>
#include <QTimer>
#include <QUuid>
#include <QtConcurrent/QtConcurrent>

#ifdef DEFINE_LOADER_PATH
#include "../../src/define.h"
#endif

class ApplicationInstancePrivate
{
    ApplicationInstance* q_ptr = nullptr;
    Q_DECLARE_PUBLIC(ApplicationInstance);

    Application* application;
    InstanceAdaptor* adapter;
    QString m_path;
    QSharedPointer<modules::ApplicationHelper::Helper> helper;
    QDateTime startupTime;
    QString m_id;
    uint32_t pid;

public:
    ApplicationInstancePrivate(ApplicationInstance* parent) : q_ptr(parent)
    {
        startupTime = QDateTime::currentDateTime();
        m_id = QString(QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Md5).toHex());
        m_path = QString("/org/deepin/dde/Application1/Instance/%1").arg(m_id);
        adapter = new InstanceAdaptor(q_ptr);
    }

    ~ApplicationInstancePrivate()
    {
        // disconnect dbus
        QDBusConnection::sessionBus().unregisterObject(m_path);
    }

    void run()
    {
#ifdef DEFINE_LOADER_PATH

        const QString task_hash{QString("DAM_TASK_HASH=%1").arg(m_id)};
        const QString task_type{"DAM_TASK_TYPE=freedesktop "};
        QProcess* p = new QProcess(q_ptr);
        p->connect(p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), p, [ = ] {
            qInfo().noquote() << p->readAllStandardOutput();
            qWarning().noquote() << p->readAllStandardError();
        });
        p->connect(p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), q_ptr, &ApplicationInstance::taskFinished);
        p->connect(p, &QProcess::readyReadStandardOutput, p, [ = ]
        { qInfo() << p->readAllStandardOutput(); });
        p->connect(p, &QProcess::readyReadStandardError, p, [ = ]
        { qWarning() << p->readAllStandardError(); });
        p->setProgram(LOADER_PATH);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("DAM_TASK_HASH", m_id);
        env.insert("DAM_TASK_TYPE", "freedesktop");
        p->setEnvironment(env.toStringList());
        p->start();
        p->waitForStarted();
        if (p->state() == QProcess::ProcessState::NotRunning) {
            Q_EMIT q_ptr->taskFinished(p->exitCode());
        }
#else
        qInfo() << "app manager load service:" << QString("org.deepin.dde.Application1.Instance@%1.service").arg(m_id);
        QDBusInterface systemd("org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager");
        QDBusReply<void> reply = systemd.call("StartUnit", QString("org.deepin.dde.Application1.Instance@%1.service").arg(m_id), "replace-irreversibly");
        if (!reply.isValid()) {
            qInfo() << reply.error();
            q_ptr->deleteLater();
        }
#endif
    }

    void _exit()
    {
#ifdef LOADER_PATH
#else
        QDBusInterface systemd("org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager");
        qInfo() << systemd.call("StopUnit", QString("org.deepin.dde.Application1.Instance@%1.service").arg(m_id), "replace-irreversibly");
#endif
    }

    void _kill() {}
    uint32_t _getPid()
    {
        return pid;
    }

    void _success(const QString& data)
    {
        pid = data.toUInt();
    }
};

ApplicationInstance::ApplicationInstance(Application* parent, QSharedPointer<modules::ApplicationHelper::Helper> helper, QStringList files)
    : QObject(nullptr)
    , dd_ptr(new ApplicationInstancePrivate(this))
    , m_files(files)
{
    Q_D(ApplicationInstance);

    d->application = parent;
    d->helper = helper;

    QTimer::singleShot(0, this, [ = ] {
        QDBusConnection::sessionBus().registerObject(d->m_path, "org.deepin.dde.Application1.Instance", this);
        d->run();
    });
}

ApplicationInstance::~ApplicationInstance()
{
    Q_D(ApplicationInstance);
    qDebug() << "instance quit " << d->helper->desktop();
}

QDBusObjectPath ApplicationInstance::id() const
{
    Q_D(const ApplicationInstance);

    return d->application->path();
}

QString ApplicationInstance::hash() const
{
    Q_D(const ApplicationInstance);

    return d->m_id;
}

quint64 ApplicationInstance::startuptime() const
{
    Q_D(const ApplicationInstance);

    return d->startupTime.toSecsSinceEpoch();
}

QDBusObjectPath ApplicationInstance::path() const
{
    Q_D(const ApplicationInstance);

    return QDBusObjectPath(d->m_path);
}

Methods::Task ApplicationInstance::taskInfo() const
{
    Q_D(const ApplicationInstance);

    Methods::Task task;
    task.id = d->m_id;
    task.runId = d->application->id();
    task.filePath = d->application->filePath();
    task.date = QString::number(startuptime());
    task.arguments = m_files;

    // TODO: debug to display environment
    task.environments.insert("DISPLAY", ":0");
    auto sysEnv = QProcessEnvironment::systemEnvironment();
    for (const auto& key : sysEnv.keys()) {
        task.environments.insert(key, sysEnv.value(key));
    }

    return task;
}

void ApplicationInstance::Exit()
{
    Q_D(ApplicationInstance);

    return d->_exit();
}

void ApplicationInstance::Kill()
{
    Q_D(ApplicationInstance);

    return d->_kill();
}

uint32_t ApplicationInstance::getPid()
{
    Q_D(ApplicationInstance);

    return d->_getPid();
}

void ApplicationInstance::Success(const QString& data)
{
    Q_D(ApplicationInstance);
    QDBusConnection::sessionBus().registerObject(d->m_path, "org.deepin.dde.Application1.Instance", this);
    return d->_success(data);
}
