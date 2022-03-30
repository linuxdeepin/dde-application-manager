#ifndef REGISTER_H_
#define REGISTER_H_

#include <nlohmann/json.hpp>

namespace Methods {
struct Registe {
    std::string date;
    std::string id;
    std::string type{ "registe" };
    std::string hash;
    bool        state;
};

using json = nlohmann::json;
inline void to_json(json &j, const Registe &registe)
{
    j = json{ { "type", registe.type }, { "id", registe.id }, { "hash", registe.hash }, { "state", registe.state }, { "date", registe.date } };
}
inline void from_json(const json &j, Registe &registe)
{
    j.at("id").get_to(registe.id);
    j.at("date").get_to(registe.date);
    j.at("hash").get_to(registe.hash);
    j.at("state").get_to(registe.state);
}
}  // namespace Methods

#endif  // REGISTER_H_
