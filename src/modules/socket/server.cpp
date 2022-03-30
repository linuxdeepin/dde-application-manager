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
class ServerPrivate : public QObject {
    Q_OBJECT
public:
    Server  *q_ptr;
    int      socket_fd;
    QThread *workThread;

Q_SIGNALS:
    void requestStart();

public:
    ServerPrivate(Server *server) : QObject(), q_ptr(server), socket_fd(-1)
    {
        if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            std::cout << "socket() failed" << std::endl;
            return;
        }

        connect(this, &ServerPrivate::requestStart, this, &ServerPrivate::work, Qt::QueuedConnection);
    }
    ~ServerPrivate() {}

    void work()
    {
        // start a thread to listen client read
        while (true) {
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
                        emit q_ptr->onReadyRead(socket, data);
                        data.clear();
                    }
                }
            });
        }
    }

    bool listen(const std::string &host)
    {
        if (socket_fd < 0) {
            return false;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        snprintf(addr.sun_path, host.size() + 1, "%s", host.c_str());

        if (remove(host.c_str()) == -1 && errno != ENOENT) {
            std::cout << "remove() failed" << std::endl;
            return false;
        }
        if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            std::cout << "bind() failed" << std::endl;
            return false;
        }

        if (::listen(socket_fd, sizeof(uint)) < 0) {
            std::cout << "listen() failed" << std::endl;
            return false;
        }

        return true;
    }

    void write(int socket, const std::vector<char> &data)
    {
        ::write(socket, data.data(), data.size());
    }

    void closeClient(int socket)
    {
        ::close(socket);
    }
};

Server::Server() : QObject(nullptr), d_ptr(new ServerPrivate(this))
{
    qRegisterMetaType<std::vector<char>>("VectorChar");
}
Server::~Server() {}
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
    emit d_ptr->requestStart();
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
