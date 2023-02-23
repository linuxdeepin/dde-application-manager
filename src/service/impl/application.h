// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef A216803F_06DD_4F40_8FD1_5BAED85905BE
#define A216803F_06DD_4F40_8FD1_5BAED85905BE

#include <QSharedPointer>
#include <QObject>
#include <QDBusObjectPath>

namespace modules {
    namespace ApplicationHelper {
        class Helper;
    }
}

class ApplicationPrivate;
class ApplicationInstance;
class Application : public QObject {
    Q_OBJECT
    QScopedPointer<ApplicationPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), Application)
public:
    enum class Type {
        System,
        User,
    };

    Application(const QString& prefix, Type type, QSharedPointer<modules::ApplicationHelper::Helper> helper);
    ~Application() override;

public: // PROPERTIES
    Q_PROPERTY(QStringList categories READ categories)
    QStringList categories() const;

    Q_PROPERTY(QString icon READ icon)
    QString icon() const;

    Q_PROPERTY(QString id READ id)
    QString id() const;

    Q_PROPERTY(QList<QDBusObjectPath> instances READ instances)
    QList<QDBusObjectPath> instances() const;

    Q_PROPERTY(QStringList mimetypes READ mimetypes)
    QStringList mimetypes() const;

    QDBusObjectPath path() const;

    QString prefix() const;

    Type type() const;

    QString filePath() const;

    QSharedPointer<ApplicationInstance> createInstance(QStringList files);
    QList<QSharedPointer<ApplicationInstance>>& getAllInstances();
    bool destoryInstance(QString hashId);

public Q_SLOTS: // METHODS
    QString Comment(const QString &locale);
    QString Name(const QString &locale);
};

#endif /* A216803F_06DD_4F40_8FD1_5BAED85905BE */
