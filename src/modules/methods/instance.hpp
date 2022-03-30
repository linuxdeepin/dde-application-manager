#ifndef C664E26D_6517_412B_950F_07E20963349E
#define C664E26D_6517_412B_950F_07E20963349E

#include <nlohmann/json.hpp>

namespace Methods {
struct Instance {
    std::string hash;
    std::string type{ "instance" };
};

using json = nlohmann::json;
inline void to_json(json &j, const Instance &instance)
{
    j = json{ { "type", instance.type }, { "hash", instance.hash } };
}

inline void from_json(const json &j, Instance &instance)
{
    j.at("hash").get_to(instance.hash);
}

}  // namespace Methods

#endif /* C664E26D_6517_412B_950F_07E20963349E */
