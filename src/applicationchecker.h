// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONCHECKER_H
#define APPLICATIONCHECKER_H

#include <QLoggingCategory>

#include "desktopentry.h"

Q_DECLARE_LOGGING_CATEGORY(DDEAMChecker)

class SessionOverrideConfig;

namespace ApplicationFilter {

bool hiddenCheck(const DesktopEntry &entry) noexcept;
bool tryExecCheck(const DesktopEntry &entry, QStringView desktopId, const SessionOverrideConfig *sessionConfig = nullptr) noexcept;
bool showInCheck(const DesktopEntry &entry) noexcept;

}  // namespace ApplicationFilter
#endif
