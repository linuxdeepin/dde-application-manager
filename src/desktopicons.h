// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DESKTOPICONS_H
#define DESKTOPICONS_H

#include "global.h"

class DesktopIcons
{
public:
    DesktopIcons() = default;
    const IconMap &icons() const noexcept { return m_icons; }
    IconMap &iconsRef() noexcept { return m_icons; }

private:
    IconMap m_icons;
};

#endif
