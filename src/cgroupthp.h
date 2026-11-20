// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef CGROUPTHP_H
#define CGROUPTHP_H

#include <QString>
#include <QStringView>

namespace CgroupThp {

// Modes accepted by the desktop kernel's memory.thp_mode interface.
bool isValidMode(QStringView mode) noexcept;

// Applies the configured THP mode to the cgroup of the given systemd unit.
// The mode is read from DConfig with the application id as subpath.
void applyForUnit(const QString &unitName, const QString &appId) noexcept;

}  // namespace CgroupThp

#endif  // CGROUPTHP_H
