// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QSettings>
#include <QThread>
#include <algorithm>

#include "../applicationhelper.h"
#include "../modules/tools/desktop_deconstruction.hpp"
#include "application_instance.h"

class ApplicationPrivate {
    Application *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(Application);

    QList<QSharedPointer<ApplicationInstance>>         instances;
    QSharedPointer<modules::ApplicationHelper::Helper> helper;
    QString                                            m_prefix;
    Application::Type                                  m_type;

public:
    ApplicationPrivate(Application *parent) : q_ptr(parent) {}

    ~ApplicationPrivate() {}

    QStringList categories() const
    {
        return helper->categories();
    }

    QString icon() const
    {
        return helper->icon();
    }

    QString id() const
    {
        return helper->id();
    }

    QStringList mimetypes() const
    {
        return helper->mimetypes();
    }

    QString comment(const QString &locale) const
    {
        return helper->comment(locale);
    }

    QString name(const QString &name) const
    {
        return helper->name(name);
    }
};

Application::Application(const QString &prefix, Type type, QSharedPointer<modules::ApplicationHelper::Helper> helper) : QObject(nullptr), dd_ptr(new ApplicationPrivate(this))
{
    Q_D(Application);

    d->helper   = helper;
    d->m_type   = type;
    d->m_prefix = prefix;
}

Application::~Application() {}

QStringList Application::categories() const
{
    Q_D(const Application);

    return d->categories();
}

QString Application::icon() const
{
    Q_D(const Application);

    return d->icon();
}

QString Application::id() const
{
    Q_D(const Application);

    const QString id{ d->id() };
    return QString("/%1/%2/%3").arg(d->m_prefix).arg(d->m_type == Application::Type::System ? "system" : "user").arg(id);
}

QList<QDBusObjectPath> Application::instances() const
{
    Q_D(const Application);

    QList<QDBusObjectPath> result;

    for (const auto &ins : d->instances) {
        result << ins->path();
    }

    return result;
}

QStringList Application::mimetypes() const
{
    Q_D(const Application);

    return d->mimetypes();
}

QString Application::Comment(const QString &locale)
{
    Q_D(const Application);

    return d->comment(locale);
}

QString Application::Name(const QString &locale)
{
    Q_D(const Application);

    return d->name(locale);
}

QDBusObjectPath Application::path() const
{
    return QDBusObjectPath(QString("/org/deepin/dde/Application1/%1").arg(QString(QCryptographicHash::hash(id().toUtf8(), QCryptographicHash::Md5).toHex())));
}

Application::Type Application::type() const
{
    Q_D(const Application);

    return d->m_type;
}

QString Application::filePath() const
{
    Q_D(const Application);

    return d->helper->desktop();
}

QSharedPointer<ApplicationInstance> Application::createInstance(QStringList files)
{
    Q_D(Application);

    d->instances << QSharedPointer<ApplicationInstance>(new ApplicationInstance(this, d->helper, files));

    connect(d->instances.last().get(), &ApplicationInstance::taskFinished, this, [=] {
        for (auto it = d->instances.begin(); it != d->instances.end(); ++it) {
            if ((*it).data() == sender()) {
                d->instances.erase(it);
                return;
            }
        }
        qWarning() << "The instance should not be found!";
    });

    return d->instances.last();
}

QString Application::prefix() const
{
    Q_D(const Application);

    return d->m_prefix;
}

QList<QSharedPointer<ApplicationInstance>>& Application::getAllInstances(){
    Q_D(Application);

    return d->instances;
}
