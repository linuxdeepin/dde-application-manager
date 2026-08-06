// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SESSIONTYPE_H
#define SESSIONTYPE_H

enum class SessionType
{
    X11,
    Wayland,
    Unknown
};

SessionType currentSessionType() noexcept;

#endif