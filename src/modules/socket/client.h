// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef B1D5EB4F_7645_4BDA_87D6_6B80A4910014
#define B1D5EB4F_7645_4BDA_87D6_6B80A4910014

#include <functional>
#include <memory>
#include <string>

#include <QJsonDocument>

namespace Socket
{
    class ClientPrivate;
    class Client
    {
        std::unique_ptr<ClientPrivate> d_ptr;

    public:
        Client();
        ~Client();
        bool connect(const std::string &host);
        QByteArray get(const QByteArray &call);
        size_t send(const QByteArray &call);
        void onReadyRead(std::function<void(const std::vector<char> &)> func);
        void waitForFinished();
    };
} // namespace Socket

#endif /* B1D5EB4F_7645_4BDA_87D6_6B80A4910014 */
