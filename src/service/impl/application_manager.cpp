#include "application_manager.h"

#include <unistd.h>

#include <QDBusMessage>
#include <QDBusConnectionInterface>
#include <QDBusConnection>
#include <QDebug>

#include <iostream>
#include <map>
#include <mutex>
#include <thread>

#include "../../modules/methods/basic.h"
#include "../../modules/methods/instance.hpp"
#include "../../modules/methods/quit.hpp"
#include "../../modules/methods/registe.hpp"
#include "../../modules/methods/task.hpp"
#include "../../modules/socket/server.h"
#include "application.h"
#include "application_instance.h"
#include "applicationinstanceadaptor.h"

class ApplicationManagerPrivate : public QObject
{
    Q_OBJECT
    ApplicationManager *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(ApplicationManager);

    QList<QSharedPointer<Application>> applications;
    Socket::Server server;
    std::multimap<std::string, QSharedPointer<ApplicationInstance>> tasks;

public:
    ApplicationManagerPrivate(ApplicationManager *parent) : QObject(parent), q_ptr(parent)
    {
        const QString socketPath{QString("/run/user/%1/deepin-application-manager.socket").arg(getuid())};
        connect(&server, &Socket::Server::onReadyRead, this, &ApplicationManagerPrivate::recvClientData, Qt::QueuedConnection);
        server.listen(socketPath.toStdString());
    }
    ~ApplicationManagerPrivate() {}

private:
    void recvClientData(int socket, const std::vector<char> &data)
    {
        std::string tmp;
        for (char c : data) {
            tmp += c;
        }

        QByteArray jsonArray = data.data();
        Methods::Basic basic;
        Methods::fromJson(jsonArray, basic);
        QByteArray tmpArray;
        do {
            if (basic.type == "instance") {
                Methods::Instance instance;
                Methods::fromJson(jsonArray, instance);
                auto find = tasks.find(instance.hash.toStdString());
                if (find != tasks.end())
                {
                    Methods::Task task = find->second->taskInfo();
                    Methods::toJson(tmpArray, task);
                    write(socket, tmpArray.toStdString());
                    tasks.erase(find);
                    break;
                }
            }
            if (basic.type == "quit") {
                Methods::Quit quit;
                Methods::fromJson(jsonArray, quit);
                server.close(socket);
                std::cout << "client quit" << std::endl;
                break;
            }
            if (basic.type == "registe") {
                Methods::Registe registe;
                Methods::fromJson(jsonArray, registe);
                Methods::Registe result;
                result.state = false;
                // std::lock_guard<std::mutex> lock(task_mutex);
                for (auto it = tasks.begin(); it != tasks.end(); ++it) {
                    result.state = true;
                    result.hash = QString::fromStdString(it->first);
                }
                Methods::toJson(tmpArray, result);
                write(socket, tmpArray.toStdString());
                break;
            }
            write(socket, jsonArray.toStdString());
        } while (false);
    }

    void write(int socket, const std::vector<char> &data)
    {
        std::vector<char> tmp = data;
        tmp.push_back('\0');
        server.write(socket, tmp);
    }
    void write(int socket, const std::string &data)
    {
        std::vector<char> result;
        std::copy(data.cbegin(), data.cend(), std::back_inserter(result));
        return write(socket, result);
    }
    void write(int socket, const char c)
    {
        return write(socket, std::vector<char>(c));
    }
};

ApplicationManager::ApplicationManager(QObject *parent)
 : QObject(parent)
 , dd_ptr(new ApplicationManagerPrivate(this))
 , startManager(new StartManager(this))
{
    connect(startManager, &StartManager::autostartChanged, this, &ApplicationManager::AutostartChanged);
}

ApplicationManager::~ApplicationManager() {}

void ApplicationManager::addApplication(const QList<QSharedPointer<Application>> &list)
{
    Q_D(ApplicationManager);

    d->applications = list;
}

QDBusObjectPath ApplicationManager::GetInformation(const QString &id)
{
    Q_D(ApplicationManager);

    if (!checkDMsgUid())
        return {};

    for (const QSharedPointer<Application> &app : d->applications) {
        if (app->id() == id) {
            return app->path();
        }
    }
    return {};
}

QList<QDBusObjectPath> ApplicationManager::GetInstances(const QString &id)
{
    Q_D(const ApplicationManager);
    if (!checkDMsgUid())
        return {};

    for (const auto &app : d->applications) {
        if (app->id() == id) {
            return app->instances();
        }
    }

    return {};
}

QDBusObjectPath ApplicationManager::Run(const QString &id)
{
    Q_D(ApplicationManager);
    if (!checkDMsgUid())
        return {};

    // 创建一个实例
    for (const QSharedPointer<Application> &app : d->applications) {
        if (app->id() == id) {
            // 创建任务所需的数据，并记录到任务队列，等待 loader 消耗
            QSharedPointer<ApplicationInstance> instance{app->createInstance()};
            const std::string hash{instance->hash().toStdString()};
            connect(instance.get(), &ApplicationInstance::taskFinished, this, [=] {
                for (auto it = d->tasks.begin(); it != d->tasks.end(); ++it) {
                    if (it->first == hash) {
                        d->tasks.erase(it);
                        break;
                    }
                }
            });
            d->tasks.insert(std::make_pair(hash, instance));
            return instance->path();
        }
    }
    return {};
}

bool ApplicationManager::AddAutostart(QString fileName)
{
    if (!checkDMsgUid())
        return false;

    return startManager->addAutostart(fileName);
}

bool ApplicationManager::RemoveAutostart(QString fileName)
{
    if (!checkDMsgUid())
        return false;

    return startManager->removeAutostart(fileName);
}

QStringList ApplicationManager::AutostartList()
{
    if (!checkDMsgUid())
        return {};

    return startManager->autostartList();
}

QString ApplicationManager::DumpMemRecord()
{
    if (!checkDMsgUid())
        return {};

    return startManager->dumpMemRecord();
}

bool ApplicationManager::IsAutostart(QString fileName)
{
    if (!checkDMsgUid())
        return false;

    return startManager->isAutostart(fileName);
}

bool ApplicationManager::IsMemSufficient()
{
    if (!checkDMsgUid())
        return true;

    return startManager->isMemSufficient();
}

void ApplicationManager::LaunchApp(QString desktopFile, uint32_t timestamp, QStringList files)
{
    if (!checkDMsgUid())
        return;

    startManager->launchApp(desktopFile, timestamp, files);
}

void ApplicationManager::LaunchAppAction(QString desktopFile, QString action, uint32_t timestamp)
{
    if (!checkDMsgUid())
        return;

    startManager->launchAppAction(desktopFile, action, timestamp);
}

void ApplicationManager::LaunchAppWithOptions(QString desktopFile, uint32_t timestamp, QStringList files, QMap<QString, QString> options)
{
    if (!checkDMsgUid())
        return;

    startManager->launchAppWithOptions(desktopFile, timestamp, files, options);
}

void ApplicationManager::RunCommand(QString exe, QStringList args)
{
    if (!checkDMsgUid())
        return;

    startManager->runCommand(exe, args);
}

void ApplicationManager::RunCommandWithOptions(QString exe, QStringList args, QMap<QString, QString> options)
{
    if (!checkDMsgUid())
        return;

    startManager->runCommandWithOptions(exe, args, options);
}

void ApplicationManager::TryAgain(bool launch)
{
    if (!checkDMsgUid())
        return;

    startManager->tryAgain(launch);
}

QList<QDBusObjectPath> ApplicationManager::instances() const
{
    Q_D(const ApplicationManager);

    QList<QDBusObjectPath> result;

    for (const auto &app : d->applications) {
        result += app->instances();
    }

    return result;
}

QList<QDBusObjectPath> ApplicationManager::list() const
{
    Q_D(const ApplicationManager);

    QList<QDBusObjectPath> result;
    for (const QSharedPointer<Application> &app : d->applications) {
        result << app->path();
    }

    return result;
}

bool ApplicationManager::checkDMsgUid()
{
    QDBusReply<uint> reply = connection().interface()->serviceUid(message().service());
    return reply.isValid() && (reply.value() == getuid());
}

#include "application_manager.moc"
