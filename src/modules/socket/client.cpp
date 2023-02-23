// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "client.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <thread>

namespace Socket {
struct ClientPrivate {
    Client*                                       q_ptr;
    int                                           socket_fd;
    std::function<void(const std::vector<char>&)> readFunc;
    std::thread*                                  readThread;

    ClientPrivate(Client* client) : q_ptr(client), readThread(nullptr) {}
    ~ClientPrivate()
    {
        if (readThread) {
            close(socket_fd);
            readThread->join();
            delete readThread;
        }
    }
    bool connect(const std::string& host)
    {
        sockaddr_un address;

        if ((socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
            printf("socket() failed\n");
            return false;
        }

        memset(&address, 0, sizeof(struct sockaddr_un));

        address.sun_family = AF_LOCAL;
        snprintf(address.sun_path, host.size() + 1, "%s", host.c_str());
        if (::connect(socket_fd, (struct sockaddr*) &address, sizeof(struct sockaddr_un)) != 0) {
            printf("connect() failed\n");
            return false;
        }

        if (readThread) {
            return false;
        }

        // TODO readFunc function?
        if (readFunc) {
            readThread = new std::thread([=] {
                char              buf[512];
                std::vector<char> result;
                int               bytesRead;
                while ((bytesRead = recv(socket_fd, buf, 512, 0)) > 0) {
                    if (bytesRead == -1) {
                        return;
                    }
                    for (int i = 0; i < bytesRead; i++) {
                        result.push_back(buf[i]);
                    }
                    if (buf[bytesRead - 1] == '\0') {
                        readFunc(result);
                        result.clear();
                    }
                };
            });
            readThread->detach();
        }

        return true;
    }

    QByteArray get(const QByteArray &call) {
        send(call);

        char buf[512];
        std::vector<char> data;
        int bytesRead;
        while ((bytesRead = recv(socket_fd, buf, 512, 0)) > 0) {
            for (int i = 0; i < bytesRead; ++i) {
                data.push_back(buf[i]);
            }

            if (buf[bytesRead - 1] == '\0') {
                break;
            }
        }

        return data.data();
    }

    size_t send(const QByteArray &call) {
        std::string data = call.data();
        // \0 作为结尾标记
        data += '\0';
        return write(socket_fd, data.c_str(), data.length());
    }
};

Client::Client() : d_ptr(new ClientPrivate(this)) {}

Client::~Client() {}

bool Client::connect(const std::string& host)
{
    return d_ptr->connect(host);
}

QByteArray Client::get(const QByteArray &call)
{
    return d_ptr->get(call);
}

size_t Client::send(const QByteArray &call)
{
    return d_ptr->send(call);
}

void Client::onReadyRead(std::function<void(const std::vector<char>&)> func)
{
    d_ptr->readFunc = func;
}

void Client::waitForFinished()
{
    d_ptr->readThread->join();
}
}  // namespace Socket
