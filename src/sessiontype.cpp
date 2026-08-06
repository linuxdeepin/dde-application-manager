// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "sessiontype.h"
#include <QProcessEnvironment>

using namespace Qt::StringLiterals;

SessionType currentSessionType() noexcept
{
    static const auto type = []() {
        const auto env = QProcessEnvironment::systemEnvironment();
        if (!env.value(u"WAYLAND_DISPLAY"_s).isEmpty()) {
            return SessionType::Wayland;
        }
        const auto sessionType = env.value(u"XDG_SESSION_TYPE"_s);
        if (sessionType.compare(u"wayland"_s, Qt::CaseInsensitive) == 0) {
            return SessionType::Wayland;
        }
        if (sessionType.compare(u"x11"_s, Qt::CaseInsensitive) == 0) {
            return SessionType::X11;
        }
        return SessionType::Unknown;
    }();
    return type;
}