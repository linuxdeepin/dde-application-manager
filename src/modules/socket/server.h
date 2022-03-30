#ifndef F358257E_94E5_4A6C_91A8_4B6E57999E7B
#define F358257E_94E5_4A6C_91A8_4B6E57999E7B

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <QObject>

namespace Socket {
class ServerPrivate;
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
