// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef TYPES_H
#define TYPES_H

#include <string_view>
#include <systemd/sd-bus.h>
#include <systemd/sd-journal.h>

enum class ExitCode : int8_t { SystemdError = -3, InvalidInput = -2, InternalError = -1, Done = 0, Waiting = 1 };

enum class DBusValueType : uint8_t { String, ArrayOfString };

using msg_ptr = sd_bus_message *;
using bus_ptr = sd_bus *;

struct JobRemoveResult
{
    std::string_view id;
    int removedFlag{0};
    ExitCode result{ExitCode::Waiting};
};

#endif
