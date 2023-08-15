// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONSERVICE_H
#define APPLICATIONSERVICE_H

#include <QObject>
#include <QDBusObjectPath>
#include <QMap>
#include <QString>
#include <QDBusUnixFileDescriptor>
#include <QSharedPointer>
#include <QUuid>
#include <QTextStream>
#include <QFile>
#include "dbus/instanceservice.h"
#include "global.h"
#include "desktopentry.h"
#include "desktopicons.h"
#include "dbus/jobmanager1service.h"

class ApplicationService : public QObject
{
    Q_OBJECT
public:
    ~ApplicationService() override;
    ApplicationService(const ApplicationService &) = delete;
    ApplicationService(ApplicationService &&) = delete;
    ApplicationService &operator=(const ApplicationService &) = delete;
    ApplicationService &operator=(ApplicationService &&) = delete;

    Q_PROPERTY(QStringList Actions READ actions)
    [[nodiscard]] QStringList actions() const noexcept;

    Q_PROPERTY(QString ID READ id CONSTANT)
    [[nodiscard]] QString id() const noexcept;

    Q_PROPERTY(IconMap Icons READ icons)
    [[nodiscard]] IconMap icons() const;
    IconMap &iconsRef();

    Q_PROPERTY(bool AutoStart READ isAutoStart WRITE setAutoStart)
    [[nodiscard]] bool isAutoStart() const noexcept;
    void setAutoStart(bool autostart) noexcept;

    Q_PROPERTY(QList<QDBusObjectPath> Instances READ instances)
    [[nodiscard]] QList<QDBusObjectPath> instances() const noexcept;

    [[nodiscard]] QDBusObjectPath findInstance(const QString &instanceId) const;

    [[nodiscard]] const QString &getLauncher() const noexcept { return m_launcher; }
    void setLauncher(const QString &launcher) noexcept { m_launcher = launcher; }

    bool addOneInstance(const QString &instanceId, const QString &application, const QString &systemdUnitPath);
    void recoverInstances(const QList<QDBusObjectPath>) noexcept;
    void removeOneInstance(const QDBusObjectPath &instance);
    void removeAllInstance();

public Q_SLOTS:
    QString GetActionName(const QString &identifier, const QStringList &env);
    QDBusObjectPath Launch(QString action, QStringList fields, QVariantMap options);

private:
    friend class ApplicationManager1Service;
    template <typename T>
    explicit ApplicationService(T &&source, ApplicationManager1Service *parent = nullptr)
        : m_parent(parent)
        , m_desktopSource(std::forward<T>(source))
    {
        static_assert(std::is_same_v<T, DesktopFile> or std::is_same_v<T, QString>, "param type must be QString or DesktopFile.");
        QString objectPath{DDEApplicationManager1ApplicationObjectPath};
        QTextStream sourceStream;
        QFile sourceFile;
        auto dbusAppid = m_desktopSource.m_file.desktopId();

        if constexpr (std::is_same_v<T, DesktopFile>) {
            m_applicationPath =
#ifdef DEBUG_MODE
                QDBusObjectPath{objectPath + escapeToObjectPath(dbusAppid)};
#else
                QDBusObjectPath{objectPath + QUuid::createUuid().toString(QUuid::Id128)};
#endif
            m_isPersistence = true;
            sourceFile.setFileName(m_desktopSource.m_file.filePath());
            if (!sourceFile.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text)) {
#ifndef DEBUG_MODE
                qCritical() << "desktop file can't open:" << m_desktopSource.m_file.filePath() << sourceFile.errorString();
#endif
                return;
            }
            sourceStream.setDevice(&sourceFile);
        } else if (std::is_same_v<T, QString>) {
            m_applicationPath = QDBusObjectPath{objectPath + QUuid::createUuid().toString(QUuid::Id128)};
            m_isPersistence = false;
            sourceStream.setString(&m_desktopSource.m_temp, QTextStream::ReadOnly | QTextStream::Text);
        }
        m_entry.reset(new DesktopEntry());
        if (auto error = m_entry->parse(sourceStream); error != DesktopErrorCode::NoError) {
            if (error != DesktopErrorCode::EntryKeyInvalid) {
                m_entry.reset(nullptr);
                return;
            }
        }
        // TODO: icon lookup
    }

    bool m_AutoStart{false};
    bool m_isPersistence;
    ApplicationManager1Service *m_parent{nullptr};
    QDBusObjectPath m_applicationPath;
    QString m_launcher{getApplicationLauncherBinary()};
    union DesktopSource
    {
        template <typename T, std::enable_if_t<std::is_same_v<T, DesktopFile>, bool> = true>
        DesktopSource(T &&source)
            : m_file(std::forward<T>(source))
        {
        }

        template <typename T, std::enable_if_t<std::is_same_v<T, QString>, bool> = true>
        DesktopSource(T &&source)
            : m_temp(std::forward<T>(source))
        {
        }

        void destruct(bool isPersistence)
        {
            if (isPersistence) {
                m_file.~DesktopFile();
            } else {
                m_temp.~QString();
            }
        }
        ~DesktopSource(){};
        QString m_temp;
        DesktopFile m_file;
    } m_desktopSource;
    QSharedPointer<DesktopEntry> m_entry{nullptr};
    QSharedPointer<DesktopIcons> m_Icons{nullptr};
    QMap<QDBusObjectPath, QSharedPointer<InstanceService>> m_Instances;
    QString userNameLookup(uid_t uid);
    [[nodiscard]] LaunchTask unescapeExec(const QString &str, const QStringList &fields);
};

#endif
