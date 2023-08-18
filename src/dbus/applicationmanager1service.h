// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONMANAGER1SERVICE_H
#define APPLICATIONMANAGER1SERVICE_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QSharedPointer>
#include <QScopedPointer>
#include <memory>
#include <QMap>
#include "dbus/jobmanager1service.h"
#include "dbus/APPobjectmanager1adaptor.h"
#include "dbus/applicationadaptor.h"
#include "identifier.h"

class ApplicationManager1Service final : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationManager1Service(std::unique_ptr<Identifier> ptr, QDBusConnection &connection);
    ~ApplicationManager1Service() override;
    ApplicationManager1Service(const ApplicationManager1Service &) = delete;
    ApplicationManager1Service(ApplicationManager1Service &&) = delete;
    ApplicationManager1Service &operator=(const ApplicationManager1Service &) = delete;
    ApplicationManager1Service &operator=(ApplicationManager1Service &&) = delete;

    Q_PROPERTY(QList<QDBusObjectPath> List READ list)
    [[nodiscard]] QList<QDBusObjectPath> list() const;

    template <typename T>
    bool addApplication(T &&desktopFileSource)
    {
        QSharedPointer<ApplicationService> application = makeApplication(std::forward<T>(desktopFileSource), this);
        if (!application) {
            return false;
        }

        if (m_applicationList.constFind(application->m_applicationPath) != m_applicationList.cend()) {
            auto info = qInfo();
            info << "this application already exists.";
            if (application->m_isPersistence) {
                info << "desktop source:" << application->m_desktopSource.m_file.filePath();
            }
            return false;
        }

        auto *ptr = application.data();
        new ApplicationAdaptor{ptr};

        if (!registerObjectToDBus(
                ptr, application->m_applicationPath.path(), getDBusInterface(QMetaType::fromType<ApplicationAdaptor>()))) {
            return false;
        }
        m_applicationList.insert(application->m_applicationPath, application);
        emit InterfacesAdded(application->m_applicationPath, getInterfacesListFromObject(ptr));

        return true;
    }
    void removeOneApplication(const QDBusObjectPath &application);
    void removeAllApplication();

    void updateApplication(const QSharedPointer<ApplicationService> &deskApp, const DesktopFile &desktopFile) noexcept;

    JobManager1Service &jobManager() noexcept { return *m_jobManager; }

public Q_SLOTS:
    QString Identify(const QDBusUnixFileDescriptor &pidfd, QDBusObjectPath &application, QDBusObjectPath &application_instance);
    void UpdateApplicationInfo(const QStringList &appIdList);
    [[nodiscard]] ObjectMap GetManagedObjects() const;

Q_SIGNALS:
    void InterfacesAdded(const QDBusObjectPath &object_path, const QStringList &interfaces);
    void InterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces);

private:
    std::unique_ptr<Identifier> m_identifier;
    QScopedPointer<JobManager1Service> m_jobManager{nullptr};
    QMap<QDBusObjectPath, QSharedPointer<ApplicationService>> m_applicationList;
};

template <typename T>
QSharedPointer<ApplicationService> makeApplication(T &&source, ApplicationManager1Service *parent)
{
    static_assert(std::is_same_v<T, DesktopFile> or std::is_same_v<T, QString>, "param type must be QString or DesktopFile.");
    QString objectPath;
    QTextStream sourceStream;
    QFile sourceFile;
    QSharedPointer<ApplicationService> app{nullptr};

    if constexpr (std::is_same_v<T, DesktopFile>) {
        DesktopFile in{std::forward<T>(source)};
        objectPath = QString{DDEApplicationManager1ObjectPath} + "/" + escapeToObjectPath(in.desktopId());
        sourceFile.setFileName(in.filePath());

        if (!sourceFile.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text)) {
            qCritical() << "desktop file can't open:" << in.filePath() << sourceFile.errorString();
            return nullptr;
        }

        app.reset(new ApplicationService{std::move(in)});
        sourceStream.setDevice(&sourceFile);
    } else if (std::is_same_v<T, QString>) {
        QString in{std::forward<T>(source)};
        objectPath = QString{DDEApplicationManager1ObjectPath} + "/" + QUuid::createUuid().toString(QUuid::Id128);

        app.reset(new ApplicationService{std::move(in)});
        sourceStream.setString(&in, QTextStream::ReadOnly | QTextStream::Text);
    }

    std::unique_ptr<DesktopEntry> entry{std::make_unique<DesktopEntry>()};
    auto error = entry->parse(sourceStream);

    if (error != DesktopErrorCode::NoError) {
        if (error != DesktopErrorCode::EntryKeyInvalid) {
            return nullptr;
        }
    }

    if (auto val = entry->value(DesktopFileEntryKey, "Hidden"); val.has_value()) {
        bool ok{false};
        if (auto hidden = val.value().toBoolean(ok); ok and hidden) {
            return nullptr;
        }
    }

    app->m_parent = parent;
    app->m_entry.reset(entry.release());
    app->m_applicationPath = QDBusObjectPath{std::move(objectPath)};

    // TODO: icon lookup
    new APPObjectManagerAdaptor{app.data()};
    return app;
}

#endif
