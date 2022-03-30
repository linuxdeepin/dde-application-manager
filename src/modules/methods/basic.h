#ifndef BASIC_H_
#define BASIC_H_

#include <nlohmann/json.hpp>

namespace Methods {
struct Basic {
  std::string type;
};

using json = nlohmann::json;
inline void from_json(const json &j, Basic &basic) {
    j.at("type").get_to(basic.type);
}

} // namespace Methods

#endif // BASIC_H_
