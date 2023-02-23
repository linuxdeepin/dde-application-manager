// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef A2862DC7_5DA3_4129_9796_671D88015BED
#define A2862DC7_5DA3_4129_9796_671D88015BED

#include "../../modules/startmanager/startmanager.h"
#include "../../modules/socket/server.h"
#include "../../modules/methods/process_status.hpp"

#include <QObject>
#include <QDBusObjectPath>
#include <QList>
#include <QMap>
#include <QDBusContext>

class Application;
class ApplicationInstance;
class ApplicationManagerPrivate : public QObject
{
    Q_OBJECT
    ApplicationManager *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(ApplicationManager);

    QList<QSharedPointer<Application>> applications;
    Socket::Server server;
    std::multimap<std::string, QSharedPointer<ApplicationInstance>> tasks;
    StartManager *startManager;
    std::vector<std::string>    virtualMachines;
    const std::string           virtualMachePath;
    const std::string           section;
    const std::string           key;

public:
    ApplicationManagerPrivate(ApplicationManager *parent);
    ~ApplicationManagerPrivate();

    // 检测调用方身份
    bool checkDMsgUid();
    void init();

private:
    void recvClientData(int socket, const std::vector<char> &data);

    void write(int socket, const std::vector<char> &data);

    void write(int socket, const std::string &data);

    void write(int socket, const char c);

    void processInstanceStatus(Methods::ProcessStatus instanceStatus);
};

class ApplicationManager : public QObject, public QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(QList<QDBusObjectPath> instances READ instances)
    Q_PROPERTY(QList<QDBusObjectPath> list READ list)
    QScopedPointer<ApplicationManagerPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), ApplicationManager)

public:
    static ApplicationManager* instance();

    void addApplication(const QList<QSharedPointer<Application>> &list);
    void launchAutostartApps();
    void processInstanceStatus(Methods::ProcessStatus instanceStatus);

Q_SIGNALS:
    void AutostartChanged(const QString &status, const QString &filePath);

public Q_SLOTS:
    bool AddAutostart(const QString &desktop);
    QStringList AutostartList();
    bool IsAutostart(const QString &fileName);
    bool RemoveAutostart(const QString &fileName);
    void Launch(const QString &desktopFile, bool withMsgCheck = true);
    void LaunchApp(const QString &desktopFile, uint32_t timestamp, const QStringList &files, bool withMsgCheck = true);
    void LaunchAppAction(const QString &desktopFile, const QString &action, uint32_t timestamp, bool withMsgCheck = true);
    void LaunchAppWithOptions(const QString &desktopFile, uint32_t timestamp, const QStringList &files, QVariantMap options);
    void RunCommand(const QString &exe, const QStringList &args);
    void RunCommandWithOptions(const QString &exe, const QStringList &args, const QVariantMap &options);

protected:
    ApplicationManager(QObject *parent = nullptr);
    ~ApplicationManager() override;

    QList<QDBusObjectPath> instances() const;
    QList<QDBusObjectPath> list() const;
    QDBusObjectPath GetInformation(const QString &id);
    QList<QDBusObjectPath> GetInstances(const QString &id);
    bool IsProcessExist(uint32_t pid);
};

#endif /* A2862DC7_5DA3_4129_9796_671D88015BED */
