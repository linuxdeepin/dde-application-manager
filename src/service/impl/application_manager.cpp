// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
#include <QProcess>

#include "../../modules/methods/basic.h"
#include "../../modules/methods/instance.hpp"
#include "../../modules/methods/quit.hpp"
#include "../../modules/methods/registe.hpp"
#include "../../modules/methods/task.hpp"
#include "../../modules/startmanager/startmanager.h"
#include "application.h"
#include "application_instance.h"
#include "instanceadaptor.h"
#include "../lib/keyfile.h"

ApplicationManagerPrivate::ApplicationManagerPrivate(ApplicationManager* parent)
    : QObject(parent)
    , q_ptr(parent)
    , startManager(new StartManager(this))
    , virtualMachePath("/usr/share/dde-daemon/supportVirsConf.ini")
    , section("AppName")
    , key("support")
{
    const QString socketPath{QString("/run/user/%1/dde-application-manager.socket").arg(getuid())};
    connect(&server, &Socket::Server::onReadyRead, this, &ApplicationManagerPrivate::recvClientData, Qt::QueuedConnection);
    server.listen(socketPath.toStdString());
}

ApplicationManagerPrivate::~ApplicationManagerPrivate()
{
}

bool ApplicationManagerPrivate::checkDMsgUid()
{
    QDBusReply<uint> reply = q_ptr->connection().interface()->serviceUid(q_ptr->message().service());
    return reply.isValid() && (reply.value() == getuid());
}

/**
 * @brief ApplicationManagerPrivate::recvClientData 接受客户端数据，进行校验
 * @param socket 客户端套接字
 * @param data 接受到客户端数据
 */
void ApplicationManagerPrivate::recvClientData(int socket, const std::vector<char>& data)
{
    QByteArray jsonArray = data.data();
    Methods::Basic basic;
    Methods::fromJson(jsonArray, basic);
    QByteArray tmpArray;
    do {
        // 运行实例
        if (basic.type == "instance") {
            Methods::Instance instance;
            Methods::fromJson(jsonArray, instance);

            // 校验实例信息
            auto find = tasks.find(instance.hash.toStdString());
            if (find != tasks.end()) {
                Methods::Task task = find->second->taskInfo();
                Methods::toJson(tmpArray, task);

                // 通过校验，传入应用启动信息
                write(socket, tmpArray.toStdString());
                tasks.erase(find);
                break;
            }
        }

        // 退出
        if (basic.type == "quit") {
            Methods::ProcessStatus quit;
            Methods::fromJson(jsonArray, quit);
            processInstanceStatus(quit);
            server.close(socket);
            std::cout << "client quit" << std::endl;
            break;
        }

        // 注册应用
        if (basic.type == "registe") {
            Methods::Registe registe;
            Methods::fromJson(jsonArray, registe);
            Methods::Registe result;
            result.state = false;
            // std::lock_guard<std::mutex> lock(task_mutex);
            for (auto it = tasks.begin(); it != tasks.end(); ++it) {
                if (registe.hash == QString::fromStdString(it->first)) {
                    result.state = true;
                    result.hash = registe.hash;
                    break;
                }
            }
            Methods::toJson(tmpArray, result);
            write(socket, tmpArray.toStdString());
            break;
        }

        if (basic.type == "success") {
            Methods::ProcessStatus processSuccess;
            Methods::fromJson(jsonArray, processSuccess);
            processInstanceStatus(processSuccess);
            std::cout << "client success" << std::endl;
            break;
        }
        write(socket, jsonArray.toStdString());
    } while (false);
}

void ApplicationManagerPrivate::write(int socket, const std::vector<char>& data)
{
    std::vector<char> tmp = data;
    tmp.push_back('\0');
    server.write(socket, tmp);
}

void ApplicationManagerPrivate::write(int socket, const std::string& data)
{
    std::vector<char> result;
    std::copy(data.cbegin(), data.cend(), std::back_inserter(result));
    return write(socket, result);
}

void ApplicationManagerPrivate::write(int socket, const char c)
{
    return write(socket, std::vector<char>(c));
}

void ApplicationManagerPrivate::init()
{
    KeyFile keyFile;
    keyFile.loadFile(virtualMachePath);
    virtualMachines = keyFile.getStrList(section, key);
    if (virtualMachines.empty()) {
        virtualMachines = {"hvm", "bochs", "virt", "vmware", "kvm", "cloud", "invented"};
    }
}

void ApplicationManagerPrivate::processInstanceStatus(Methods::ProcessStatus instanceStatus)
{
    bool bFound = false;

    auto application = applications.begin();
    while (application != applications.end()) {

        auto instance = (*application)->getAllInstances().begin();
        while (instance != (*application)->getAllInstances().end()) {
            if ((*instance)->hash() == instanceStatus.id) {
                bFound = true;
            }
            if (bFound) {
                if (instanceStatus.type == "success") {
                    (*instance)->Success(instanceStatus.data);
                } else if (instanceStatus.type == "quit") {
                    (*instance)->Exit();
                    (*application)->getAllInstances().erase(instance);
                } else {
                    qWarning() << "instance tyep : " << instanceStatus.type << "not found";
                }
                return;
            }
            instance++;
        }
        application++;
    }
}

ApplicationManager* ApplicationManager::instance()
{
    static ApplicationManager manager;
    return &manager;
}

ApplicationManager::ApplicationManager(QObject* parent)
    : QObject(parent)
    , dd_ptr(new ApplicationManagerPrivate(this))
{
    Q_D(ApplicationManager);

    connect(d->startManager, &StartManager::autostartChanged, this, &ApplicationManager::AutostartChanged);
}

ApplicationManager::~ApplicationManager() {}

void ApplicationManager::addApplication(const QList<QSharedPointer<Application>>& list)
{
    Q_D(ApplicationManager);

    d->applications = list;
}

/**
 * @brief ApplicationManager::launchAutostartApps 加载自启动应用
 * TODO 待优化点： 多个loader使用同一个套接字通信，串行执行，效率低
 */
void ApplicationManager::launchAutostartApps()
{
    /*
    Launch("/freedesktop/system/seahorse", QStringList());
    QTimer::singleShot(1000, [&] {
        for (auto app : startManager->autostartList()) {
            QString id = app.split("/").last().split(".").first();
            Launch(id, QStringList());
        }
    });
    */
}

QDBusObjectPath ApplicationManager::GetInformation(const QString& id)
{
    Q_D(ApplicationManager);

    if (!d->checkDMsgUid())
        return {};

    for (const QSharedPointer<Application>& app : d->applications) {
        if (app->id() == id) {
            return app->path();
        }
    }
    return {};
}

QList<QDBusObjectPath> ApplicationManager::GetInstances(const QString& id)
{
    Q_D(ApplicationManager);
    if (!d->checkDMsgUid())
        return {};

    for (const auto& app : d->applications) {
        if (app->id() == id) {
            return app->instances();
        }
    }

    return {};
}

bool ApplicationManager::AddAutostart(const QString &desktop)
{
    Q_D(ApplicationManager);
    if (!d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return false;
    }

    if (!d->startManager->addAutostart(desktop)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
        return false;
    }

    return true;
}

bool ApplicationManager::RemoveAutostart(const QString &fileName)
{
    Q_D(ApplicationManager);
    if (!d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return false;
    }

    if (!d->startManager->removeAutostart(fileName)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
        return false;
    }

    return true;
}

QStringList ApplicationManager::AutostartList()
{
    Q_D(ApplicationManager);
    if (!d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return QStringList();
    }

    return d->startManager->autostartList();
}

bool ApplicationManager::IsAutostart(const QString &fileName)
{
    Q_D(ApplicationManager);
    if (!d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return false;
    }

    if (!d->startManager->isAutostart(fileName)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
        return false;
    }

    return true;
}

void ApplicationManager::Launch(const QString &desktopFile, bool withMsgCheck)
{
    Q_D(ApplicationManager);
    if (withMsgCheck && !d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return;
    }

    if (!d->startManager->launchApp(desktopFile)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
    }
}


void ApplicationManager::LaunchApp(const QString &desktopFile, uint32_t timestamp, const QStringList &files, bool withMsgCheck)
{
    Q_D(ApplicationManager);
    if (withMsgCheck && !d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return;
    }

    if (!d->startManager->launchApp(desktopFile, timestamp, files)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
    }
}

void ApplicationManager::LaunchAppAction(const QString &desktopFile, const QString &action, uint32_t timestamp, bool withMsgCheck)
{
    Q_D(ApplicationManager);
    if (withMsgCheck && !d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return;
    }

    if (!d->startManager->launchAppAction(desktopFile, action, timestamp)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
    }
}

void ApplicationManager::LaunchAppWithOptions(const QString &desktopFile, uint32_t timestamp, const QStringList &files, QVariantMap options)
{
    Q_D(ApplicationManager);
    if (!d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return;
    }

    if (!d->startManager->launchAppWithOptions(desktopFile, timestamp, files, options)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
    }
}

void ApplicationManager::RunCommand(const QString &exe, const QStringList &args)
{
    Q_D(ApplicationManager);
    if (!d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return;
    }

    if (!d->startManager->runCommand(exe, args)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
    }
}

void ApplicationManager::RunCommandWithOptions(const QString &exe, const QStringList &args, const QVariantMap &options)
{
    Q_D(ApplicationManager);
    if (!d->checkDMsgUid()) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, "The call failed");

        qWarning() << "check msg failed...";
        return;
    }

    if (!d->startManager->runCommandWithOptions(exe, args, options)) {
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, "invalid arguments");

        qWarning() << "invalid arguments";
    }
}

QList<QDBusObjectPath> ApplicationManager::instances() const
{
    Q_D(const ApplicationManager);

    QList<QDBusObjectPath> result;

    for (const auto& app : d->applications) {
        result += app->instances();
    }

    return result;
}

QList<QDBusObjectPath> ApplicationManager::list() const
{
    Q_D(const ApplicationManager);

    QList<QDBusObjectPath> result;
    for (const QSharedPointer<Application>& app : d->applications) {
        result << app->path();
    }

    return result;
}

bool ApplicationManager::IsProcessExist(uint32_t pid)
{
    Q_D(const ApplicationManager);

    for (auto app : d->applications) {
        for (auto instance : app->getAllInstances()) {
            if (instance->getPid() == pid) {
                return true;
            }
        }
    }

    return false;
}

#include "application_manager.moc"
