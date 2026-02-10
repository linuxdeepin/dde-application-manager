// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "variantValue.h"

Handler creatValueHandler(msg_ptr msg, DBusValueType type) noexcept
{
    switch (type) {
    case DBusValueType::String:
        return StringHandler{msg};
    case DBusValueType::ArrayOfString:
        return ASHandler{msg};
    default:
        return std::monostate{};
    }
}

int StringHandler::openVariant() noexcept
{
    return sd_bus_message_open_container(m_msg, SD_BUS_TYPE_VARIANT, "s");
}

int StringHandler::closeVariant() noexcept
{
    return sd_bus_message_close_container(m_msg);
}

int StringHandler::appendValue(std::string_view value) noexcept
{
    return sd_bus_message_append(m_msg, "s", value.data());
}

int ASHandler::openVariant() noexcept
{
    if (const auto ret = sd_bus_message_open_container(m_msg, SD_BUS_TYPE_VARIANT, "as"); ret < 0) {
        return ret;
    }

    return sd_bus_message_open_container(m_msg, SD_BUS_TYPE_ARRAY, "s");
}

int ASHandler::closeVariant() noexcept
{
    if (const auto ret = sd_bus_message_close_container(m_msg); ret < 0) {
        return ret;
    }

    return sd_bus_message_close_container(m_msg);
}

int ASHandler::appendValue(std::string_view value) noexcept
{
    return sd_bus_message_append(m_msg, "s", value.data());
}
