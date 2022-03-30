#ifndef B1D5EB4F_7645_4BDA_87D6_6B80A4910014
#define B1D5EB4F_7645_4BDA_87D6_6B80A4910014

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace Socket {
class ClientPrivate;
class Client {
    std::unique_ptr<ClientPrivate> d_ptr;

public:
    Client();
    ~Client();
    bool           connect(const std::string& host);
    nlohmann::json get(const nlohmann::json& call);
    size_t         send(const nlohmann::json& call);
    void           onReadyRead(std::function<void(const std::vector<char>&)> func);
    void           waitForFinished();
};
}  // namespace Socket

#endif /* B1D5EB4F_7645_4BDA_87D6_6B80A4910014 */
