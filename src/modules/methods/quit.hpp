#ifndef QUIT_H_
#define QUIT_H_

#include <nlohmann/json.hpp>

namespace Methods {
struct Quit {
    std::string date;
    std::string type{ "quit" };
    std::string id;
    int         code;
};
using json = nlohmann::json;
inline void to_json(json &j, const Quit &quit)
{
    j = json{ { "type", quit.type }, { "date", quit.date }, { "id", quit.id }, { "code", quit.code } };
}

inline void from_json(const json &j, Quit &quit)
{
    j.at("id").get_to(quit.id);
    j.at("date").get_to(quit.date);
    j.at("code").get_to(quit.code);
}
}  // namespace Methods

#endif  // QUIT_H_
