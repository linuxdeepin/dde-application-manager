#ifndef B0B88BD6_CF1E_4E87_926A_E6DBE6B9B19C
#define B0B88BD6_CF1E_4E87_926A_E6DBE6B9B19C

#include <map>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

namespace Methods {
struct Task {
    std::string                             id;
    std::string                             runId;
    std::string                             type{ "task" };
    std::string                             date;
    std::vector<std::string>                arguments;
    std::multimap<std::string, std::string> environments;
};

using json = nlohmann::json;
inline void to_json(json &j, const Task &task)
{
    j = json{ { "type", task.type }, { "id", task.id }, { "run_id", task.runId }, { "date", task.date }, { "arguments", task.arguments }, { "environments", task.environments } };
}

inline void from_json(const json &j, Task &task)
{
    j.at("id").get_to(task.id);
    j.at("run_id").get_to(task.runId);
    j.at("date").get_to(task.date);
    j.at("arguments").get_to(task.arguments);
    j.at("environments").get_to(task.environments);
}

}  // namespace Methods

#endif /* B0B88BD6_CF1E_4E87_926A_E6DBE6B9B19C */
