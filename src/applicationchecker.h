// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONCHECKER_H
#define APPLICATIONCHECKER_H

#include "desktopentry.h"

Q_DECLARE_LOGGING_CATEGORY(DDEAMChecker)

namespace ApplicationFilter {

bool hiddenCheck(const DesktopEntry &entry) noexcept;
bool tryExecCheck(const DesktopEntry &entry) noexcept;
bool showInCheck(const DesktopEntry &entry) noexcept;

}  // namespace ApplicationFilter
#endif
