// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef F358257E_94E5_4A6C_91A8_4B6E57999E7B
#define F358257E_94E5_4A6C_91A8_4B6E57999E7B

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <QObject>

namespace Socket {

class Server;
class ServerPrivate : public QObject {
    Q_OBJECT
public:
    Server  *q_ptr;
    int      socket_fd;
    QThread *workThread;

Q_SIGNALS:
    void requestStart();

public:
    ServerPrivate(Server *server);
    ~ServerPrivate();

    void work();
    bool listen(const std::string &host);
    void write(int socket, const std::vector<char> &data);
    void closeClient(int socket);
};


class Server : public QObject {
    Q_OBJECT
    std::unique_ptr<ServerPrivate> d_ptr;

public:
    Server();
    ~Server();
    bool listen(const std::string& host);
    void write(int socket, const std::vector<char>& data);
    void close(int socket);
Q_SIGNALS:
    void onReadyRead(int socket, const std::vector<char>& data) const;
};
}  // namespace Socket

#endif /* F358257E_94E5_4A6C_91A8_4B6E57999E7B */
