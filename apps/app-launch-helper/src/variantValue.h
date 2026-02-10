// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef VARIANTVALUE_H
#define VARIANTVALUE_H

#include "types.h"
#include <variant>

class StringHandler
{
public:
    explicit StringHandler(msg_ptr msg)
        : m_msg(msg)
    {
    }
    int openVariant() noexcept;
    int closeVariant() noexcept;
    int appendValue(std::string_view value) noexcept;

private:
    msg_ptr m_msg;
};

class ASHandler
{
public:
    explicit ASHandler(msg_ptr msg)
        : m_msg(msg)
    {
    }
    int openVariant() noexcept;
    int closeVariant() noexcept;
    int appendValue(std::string_view value) noexcept;

private:
    msg_ptr m_msg;
};

using Handler = std::variant<StringHandler, ASHandler, std::monostate>;
Handler creatValueHandler(msg_ptr msg, DBusValueType type) noexcept;

#endif
