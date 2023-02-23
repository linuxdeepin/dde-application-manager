// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "server.h"

#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <iostream>

namespace Socket {

ServerPrivate::ServerPrivate(Server *server)
 : QObject()
 , q_ptr(server)
 , socket_fd(-1)
 , workThread(nullptr)
{
    if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        std::cout << "socket() failed" << std::endl;
        return;
    }

    connect(this, &ServerPrivate::requestStart, this, &ServerPrivate::work, Qt::QueuedConnection);
}

ServerPrivate::~ServerPrivate()
{

}

void ServerPrivate::work()
{
    // 处理客户端数据
    while (true) {
        // 阻塞等待客户端连接
        int socket = accept(socket_fd, nullptr, nullptr);
        if (socket == -1) {
            std::cout << "accept() failed" << std::endl;
            return;
        }
        QtConcurrent::run([=] {
            int               readBytes = 0;
            char              buffer[1024];
            std::vector<char> data;
            while (true) {
                readBytes = recv(socket, buffer, 1024, 0);
                if (readBytes == -1) {
                    std::cout << "client connect closed" << std::endl;
                    break;
                }

                if (readBytes == 0) {
                    break;
                }

                for (int i = 0; i != readBytes; i++) {
                    data.push_back(buffer[i]);
                }

                if (buffer[readBytes - 1] == '\0') {
                    Q_EMIT q_ptr->onReadyRead(socket, data);
                    data.clear();
                }
            }
        });
    }
}

bool ServerPrivate::listen(const std::string &host)
{
    if (socket_fd < 0) {
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, host.size() + 1, "%s", host.c_str());

    // 移除原有套接字文件
    if (remove(host.c_str()) == -1 && errno != ENOENT) {
        std::cout << "remove() failed" << std::endl;
        return false;
    }

    // 绑定套接字文件
    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        std::cout << "bind() failed" << std::endl;
        return false;
    }

    // 监听客户端连接
    if (::listen(socket_fd, 20) < 0) {
        std::cout << "listen() failed" << std::endl;
        return false;
    }

    return true;
}

void ServerPrivate::write(int socket, const std::vector<char> &data)
{
    ::write(socket, data.data(), data.size());
}

void ServerPrivate::closeClient(int socket)
{
    ::close(socket);
}


Server::Server()
    : QObject(nullptr)
    , d_ptr(new ServerPrivate(this))
{
    qRegisterMetaType<std::vector<char>>("VectorChar");
}

Server::~Server()
{

}

bool Server::listen(const std::string &host)
{
    if (d_ptr->workThread) {
        return false;
    }

    const bool result = d_ptr->listen(host);
    if (!result) {
        return result;
    }

    d_ptr->workThread = new QThread;
    d_ptr->moveToThread(d_ptr->workThread);
    d_ptr->workThread->start();
    Q_EMIT d_ptr->requestStart();
    return result;
}

void Server::write(int socket, const std::vector<char> &data)
{
    d_ptr->write(socket, data);
}

void Server::close(int socket)
{
    d_ptr->closeClient(socket);
}

}  // namespace Socket

#include "server.moc"
