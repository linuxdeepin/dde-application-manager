#include "application_manager.h"

#include <unistd.h>

#include <iostream>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
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

class ApplicationManagerPrivate : public QObject {
    Q_OBJECT
    ApplicationManager *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(ApplicationManager);

    QList<QSharedPointer<Application>>                              applications;
    Socket::Server                                                  server;
    std::multimap<std::string, QSharedPointer<ApplicationInstance>> tasks;

public:
    ApplicationManagerPrivate(ApplicationManager *parent) : QObject(parent), q_ptr(parent)
    {
        const QString socketPath{ QString("/run/user/%1/deepin-application-manager.socket").arg(getuid()) };
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
        using namespace nlohmann;
        if (json::parse(tmp).is_null()) {
            server.close(socket);
            return;
        }
        Methods::Basic basic = json::parse(tmp);
        do {
            if (basic.type == "instance") {
                Methods::Instance instance = nlohmann::json::parse(tmp);
                auto              find     = tasks.find(instance.hash);
                if (find != tasks.end()) {
                    nlohmann::json result = find->second->taskInfo();
                    write(socket, result.dump());
                    tasks.erase(find);
                    break;
                }
            }
            if (basic.type == "quit") {
                Methods::Quit quit = json::parse(tmp);
                server.close(socket);
                std::cout << "client quit" << std::endl;
                break;
            }
            if (basic.type == "registe") {
                Methods::Registe registe = nlohmann::json::parse(tmp);
                Methods::Registe result;
                result.state = false;
                //std::lock_guard<std::mutex> lock(task_mutex);
                for (auto it = tasks.begin(); it != tasks.end(); ++it) {
                    result.state = true;
                    result.hash  = it->first;
                }
                write(socket, json(result).dump());
                std::cout << "registe a new client" << std::endl;
                break;
            }
            write(socket, json().dump());
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

ApplicationManager::ApplicationManager(QObject *parent) : QObject(parent), dd_ptr(new ApplicationManagerPrivate(this)) {}

ApplicationManager::~ApplicationManager() {}

void ApplicationManager::addApplication(const QList<QSharedPointer<Application>> &list)
{
    Q_D(ApplicationManager);

    d->applications = list;
}

QDBusObjectPath ApplicationManager::GetId(int pid)
{
    return {};
}

QDBusObjectPath ApplicationManager::GetInformation(const QString &id)
{
    Q_D(ApplicationManager);

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

    // 创建一个实例
    for (const QSharedPointer<Application> &app : d->applications) {
        if (app->id() == id) {
            // 创建任务所需的数据，并记录到任务队列，等待 loader 消耗
            QSharedPointer<ApplicationInstance> instance{ app->createInstance() };
            const std::string                   hash{ instance->hash().toStdString() };
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

#include "application_manager.moc"
