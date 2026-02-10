// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "constant.h"
#include "types.h"
#include "variantValue.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace {

// systemd escape rules:
// https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html#Specifiers
// https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html#Command%20lines
// But we only escape:
// 1. $
// 2. An argument solely consisting of ";"
// '%' is not needed, see:
// https://github.com/systemd/systemd/blob/eefb46c83b130ccec16891c3dd89aa4f32229e80/src/core/dbus-execute.c#L1769
void encodeArgument(std::string &arg)
{
    if (arg == ";") {
        arg = R"(\;)";
        return;
    }

    auto extra = std::count(arg.cbegin(), arg.cend(), '$');
    if (extra == 0) {
        return;
    }

    const auto oldSize = arg.size();
    const auto newSize = oldSize + extra;
    arg.resize(newSize);

    auto i{oldSize};
    auto j{newSize};
    while (i > 0) {
        const auto c = arg[--i];
        if (c == '$') {
            arg[--j] = '$';
            arg[--j] = '$';
        } else {
            arg[--j] = c;
        }
    }
}

ExitCode fromString(std::string_view str)
{
    if (str == "done") {
        return ExitCode::Done;
    }

    if (str == "canceled" || str == "timeout" || str == "failed" || str == "dependency" || str == "skipped") {
        return ExitCode::SystemdError;
    }

    if (str == "internalError") {
        return ExitCode::InternalError;
    }

    if (str == "invalidInput") {
        return ExitCode::InvalidInput;
    }

    __builtin_unreachable();
}

ExitCode fromString(const char *str)
{
    if (str == nullptr) {
        return ExitCode::Waiting;
    }

    return fromString(std::string_view{str});
}

[[noreturn]] void releaseRes(sd_bus_error &error, msg_ptr &msg, bus_ptr &bus, ExitCode ret)
{
    sd_bus_error_free(&error);
    sd_bus_message_unref(msg);
    sd_bus_unref(bus);

    std::exit(static_cast<int>(ret));
}

int processExecStart(msg_ptr msg,
                     std::vector<std::string_view>::const_iterator begin,
                     std::vector<std::string_view>::const_iterator end)
{
    if (begin == end) {
        return -EINVAL;
    }

    int ret{0};

    if (ret = sd_bus_message_open_container(msg, SD_BUS_TYPE_STRUCT, "sv"); ret < 0) {
        sd_journal_perror("open struct of ExecStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_append(msg, "s", "ExecStart"); ret < 0) {
        sd_journal_perror("append ExecStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_open_container(msg, SD_BUS_TYPE_VARIANT, "a(sasb)"); ret < 0) {
        sd_journal_perror("open variant of execStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_open_container(msg, SD_BUS_TYPE_ARRAY, "(sasb)"); ret < 0) {
        sd_journal_perror("open array of execStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_open_container(msg, SD_BUS_TYPE_STRUCT, "sasb"); ret < 0) {
        sd_journal_perror("open struct of execStart failed.");
        return ret;
    }

    std::string buffer{*begin};
    encodeArgument(buffer);
    if (ret = sd_bus_message_append(msg, "s", buffer.c_str()); ret < 0) {
        sd_journal_perror("append binary of execStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_open_container(msg, SD_BUS_TYPE_ARRAY, "s"); ret < 0) {
        sd_journal_perror("open array of execStart variant failed.");
        return ret;
    }

    for (auto it = begin; it != end; ++it) {
        buffer = *it;
        encodeArgument(buffer);
        sd_journal_print(LOG_INFO, "after encode: %s", buffer.c_str());
        if (ret = sd_bus_message_append(msg, "s", buffer.c_str()); ret < 0) {
            sd_journal_perror("append args of execStart failed.");
            return ret;
        }
    }

    if (ret = sd_bus_message_close_container(msg); ret < 0) {
        sd_journal_perror("close array of execStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_append(msg, "b", 0);
        ret < 0) {  // this value indicate that systemd should be considered a failure if the process exits uncleanly
        sd_journal_perror("append boolean of execStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_close_container(msg); ret < 0) {
        sd_journal_perror("close struct of execStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_close_container(msg); ret < 0) {
        sd_journal_perror("close array of execStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_close_container(msg); ret < 0) {
        sd_journal_perror("close variant of execStart failed.");
        return ret;
    }

    if (ret = sd_bus_message_close_container(msg); ret < 0) {
        sd_journal_perror("close struct of execStart failed.");
        return ret;
    }

    return 0;
}

DBusValueType getPropType(std::string_view key)
{
    struct Entry
    {
        std::string_view k;
        DBusValueType v;
    };

    // small data just use array and linear search
    static constexpr std::array<Entry, 4> map{Entry{"Environment", DBusValueType::ArrayOfString},
                                              Entry{"UnsetEnvironment", DBusValueType::ArrayOfString},
                                              Entry{"ExecSearchPath", DBusValueType::ArrayOfString},
                                              Entry{"WorkingDirectory", DBusValueType::String}};

    for (const auto &entry : map) {
        if (entry.k == key) {
            return entry.v;
        }
    }

    return DBusValueType::String;
}

int appendPropValue(msg_ptr &msg, DBusValueType type, const std::vector<std::string> &value)
{
    auto handler = creatValueHandler(msg, type);
    return std::visit(
        [&value](auto &impl) {
            if constexpr (std::is_same_v<std::decay_t<decltype(impl)>, std::monostate>) {
                sd_journal_perror("unknown type of property's variant.");
                return -1;
            } else {
                auto ret = impl.openVariant();
                if (ret < 0) {
                    sd_journal_perror("open property's variant value failed.");
                    return ret;
                }

                for (const auto &v : value) {
                    ret = impl.appendValue(v);
                    if (ret < 0) {
                        return ret;
                    }
                }

                ret = impl.closeVariant();
                if (ret < 0) {
                    sd_journal_perror("close property's variant value failed.");
                    return ret;
                }

                return 0;
            }
        },
        handler);
}

int processKVPair(msg_ptr msg, std::unordered_map<std::string, std::vector<std::string>> &props)
{
    int ret{0};
    for (auto &[key, value] : props) {
        if (ret = sd_bus_message_open_container(msg, SD_BUS_TYPE_STRUCT, "sv"); ret < 0) {
            sd_journal_perror("open struct of properties failed.");
            return ret;
        }

        sd_journal_print(LOG_INFO, "key:%s", key.data());
        if (ret = sd_bus_message_append(msg, "s", key.c_str()); ret < 0) {
            sd_journal_perror("append key of property failed.");
            return ret;
        }

        if (ret = appendPropValue(msg, getPropType(key), value); ret < 0) {
            sd_journal_perror("append value of property failed.");
            return ret;
        }

        if (ret = sd_bus_message_close_container(msg); ret < 0) {
            sd_journal_perror("close struct of properties failed.");
            return ret;
        }
    }

    return 0;
}

std::optional<std::string> cmdParse(msg_ptr &msg, const std::vector<std::string_view> &cmdLines)
{
    std::string unitName;
    std::unordered_map<std::string, std::vector<std::string>> props;

    size_t cursor{0};
    const auto total{cmdLines.size()};
    while (cursor < total) {
        auto str = cmdLines[cursor];
        if (str == "--") {
            ++cursor;
            break;
        }

        if (str.size() < 3 || str.compare(0, 2, "--") != 0) {
            sd_journal_print(LOG_WARNING, "Unknown option: %s", str.data());
            return std::nullopt;
        }

        ++cursor;
        auto kvStr = str.substr(2);
        auto eqPos = kvStr.find('=');

        if (eqPos == std::string_view::npos || eqPos == 0) {
            sd_journal_print(LOG_WARNING, "invalid k-v pair: %s", kvStr.data());
            return std::nullopt;
        }

        std::string key{kvStr.substr(0, eqPos)};
        if (key == "Type") {
            // NOTE:
            // Systemd service type must be "exec",
            // this should not be configured in command line arguments.
            sd_journal_print(LOG_WARNING, "Type should not be configured in command line arguments.");
            continue;
        }

        auto value = kvStr.substr(eqPos + 1);
        if (key == "unitName") {
            unitName = value;
            continue;
        }

        if (key == "ExecSearchPath") {
            const std::filesystem::path path{value};

            if (!path.is_absolute()) {
                sd_journal_print(LOG_WARNING, "ExecSearchPath ignoring relative path: %s", value.data());
                continue;
            }

            props[std::move(key)].emplace_back(path.lexically_normal());
            continue;
        }

        props[std::move(key)].emplace_back(value);
    }

    // Processing of the binary file and its parameters that am want to launch
    if (unitName.empty() || cursor >= total) {
        sd_journal_print(LOG_ERR, "Missing unitName or execution arguments.");
        return "invalidInput";
    }

    int ret{0};
    if (ret = sd_bus_message_append(msg, "ss", unitName.c_str(), "replace"); ret < 0) {  // unitName and start mode
        sd_journal_perror("append unitName failed.");
        return std::nullopt;
    }

    // process properties: a(sv)
    if (ret = sd_bus_message_open_container(msg, SD_BUS_TYPE_ARRAY, "(sv)"); ret < 0) {
        sd_journal_perror("open array failed.");
        return std::nullopt;
    }

    if (ret = sd_bus_message_append(msg,
                                    "(sv)(sv)(sv)(sv)",
                                    "Type",
                                    "s",
                                    "exec",
                                    "ExitType",
                                    "s",
                                    "cgroup",
                                    "Slice",
                                    "s",
                                    "app.slice",
                                    "CollectMode",
                                    "s",
                                    "inactive-or-failed");
        ret < 0) {
        sd_journal_perror("failed to append necessary properties.");
        return std::nullopt;
    }

    if (ret = processKVPair(msg, props); ret < 0) {  // process props
        return std::nullopt;
    }

    if (ret = processExecStart(msg, cmdLines.cbegin() + cursor, cmdLines.cend()); ret < 0) {
        return std::nullopt;
    }

    if (ret = sd_bus_message_close_container(msg); ret < 0) {
        sd_journal_perror("close array failed.");
        return std::nullopt;
    }

    // append aux, it's unused for now
    if (ret = sd_bus_message_append(msg, "a(sa(sv))", 0); ret < 0) {
        sd_journal_perror("append aux failed.");
        return std::nullopt;
    }

    return unitName;
}

int jobRemovedReceiver(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    int ret{0};
    if (ret = sd_bus_error_is_set(ret_error); ret != 0) {
        sd_journal_print(LOG_ERR, "JobRemoved error: [%s,%s]", ret_error->name, ret_error->message);
    } else {
        const char *serviceId{nullptr};
        const char *jobResult{nullptr};
        if (ret = sd_bus_message_read(m, "uoss", nullptr, nullptr, &serviceId, &jobResult); ret < 0) {
            sd_journal_perror("read from JobRemoved failed.");
        } else {
            auto *ptr = reinterpret_cast<JobRemoveResult *>(userdata);
            if (ptr->id == serviceId) {
                ptr->removedFlag = 1;
                ptr->result = fromString(jobResult);
            }
        }
    }

    if (ret_error != nullptr && ret_error->_need_free != 0) {
        sd_bus_error_free(ret_error);
    }

    return ret;
}

int process_dbus_message(sd_bus *bus)
{
    int ret{0};
    ret = sd_bus_process(bus, nullptr);
    if (ret < 0) {
        sd_journal_perror("event loop error.");
        return ret;
    }

    if (ret > 0) {
        return 0;
    }

    ret = sd_bus_wait(bus, std::numeric_limits<uint64_t>::max());
    if (ret < 0) {
        sd_journal_print(LOG_ERR, "sd-bus event loop error:%s", strerror(-ret));
        return ret;
    }

    return ret;
}

}  // namespace

int main(int argc, const char *argv[])
{
    sd_bus_error error{SD_BUS_ERROR_NULL};
    sd_bus_message *msg{nullptr};
    sd_bus *bus{nullptr};
    int ret{0};

    if (ret = sd_bus_open_user(&bus); ret < 0) {
        sd_journal_perror("Failed to connect to user bus.");
        releaseRes(error, msg, bus, ExitCode::InternalError);
    }

    if (ret = sd_bus_message_new_method_call(
            bus, &msg, SystemdService, SystemdObjectPath, SystemdInterfaceName, "StartTransientUnit");
        ret < 0) {
        sd_journal_perror("Failed to create D-Bus call message");
        releaseRes(error, msg, bus, ExitCode::InternalError);
    }

    std::vector<std::string_view> args;
    args.reserve(argc);
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    auto serviceId = cmdParse(msg, args);
    if (!serviceId) {
        releaseRes(error, msg, bus, ExitCode::InternalError);
    }

    const char *path{nullptr};
    JobRemoveResult resultData{serviceId.value()};

    if (ret = sd_bus_match_signal(
            bus, nullptr, SystemdService, SystemdObjectPath, SystemdInterfaceName, "JobRemoved", jobRemovedReceiver, &resultData);
        ret < 0) {
        sd_journal_perror("add signal matcher failed.");
        releaseRes(error, msg, bus, ExitCode::InternalError);
    }

    sd_bus_message *reply{nullptr};
    if (ret = sd_bus_call(bus, msg, 0, &error, &reply); ret < 0) {
        sd_bus_error_get_errno(&error);
        sd_journal_print(LOG_ERR, "failed to call StartTransientUnit: [%s,%s]", error.name, error.message);
        releaseRes(error, msg, bus, ExitCode::InternalError);
    } else {
        sd_journal_print(LOG_INFO, "call StartTransientUnit successfully, service ID: %s", serviceId->c_str());
    }

    if (ret = sd_bus_message_read(reply, "o", &path); ret < 0) {
        sd_journal_perror("failed to parse response message.");
        sd_bus_message_unref(reply);
        releaseRes(error, msg, bus, ExitCode::InternalError);
    }

    sd_bus_message_unref(reply);

    while (true) {  // wait for jobRemoved
        const auto ret = process_dbus_message(bus);
        if (ret < 0) {
            releaseRes(error, msg, bus, ExitCode::InternalError);
        }

        if (resultData.removedFlag != 0) {
            break;
        }
    }

    releaseRes(error, msg, bus, resultData.result);
}
