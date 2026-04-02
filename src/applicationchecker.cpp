// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"
#include "constant.h"
#include "applicationchecker.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(DDEAMChecker, "dde.am.checker")
using namespace Qt::StringLiterals;

namespace {

bool hasDesktopIntersection(QStringView rawValue, const QStringList &currentDesktops) noexcept
{
    if (rawValue.isEmpty() || currentDesktops.isEmpty()) {
        return false;
    }

    const auto &dde = fromStaticRaw(DesktopDDE);
    const auto &deepin = fromStaticRaw(DesktopDeepin);

    bool currentHasDDE{false};
    for (const auto &s : currentDesktops) {
        if (s.compare(dde, Qt::CaseInsensitive) == 0 || s.compare(deepin, Qt::CaseInsensitive) == 0) {
            currentHasDDE = true;
            break;
        }
    }

    auto tokens = qTokenize(rawValue, u';', Qt::SkipEmptyParts);
    for (const auto token : tokens) {
        const bool tokenIsDDE = (token.compare(dde, Qt::CaseInsensitive) == 0 || token.compare(deepin, Qt::CaseInsensitive) == 0);

        if (tokenIsDDE) {
            if (currentHasDDE) {
                return true;
            }
        } else {
            for (const auto &current : currentDesktops) {
                if (current.compare(token, Qt::CaseInsensitive) == 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

}  // namespace

bool ApplicationFilter::hiddenCheck(const DesktopEntry &entry) noexcept
{
    bool hidden{false};
    auto hiddenVal = entry.value(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryHidden));

    if (hiddenVal.has_value()) {
        bool ok{false};
        hidden = toBoolean(hiddenVal.value(), ok);
        if (!ok) {
            qCWarning(DDEAMChecker) << "invalid hidden value:" << hiddenVal.value().get();
            return false;
        }
    }
    return hidden;
}

bool ApplicationFilter::tryExecCheck(const DesktopEntry &entry) noexcept
{
    auto tryExecVal = entry.value(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryTryExec));
    if (tryExecVal.has_value()) {
        auto executable = toString(tryExecVal.value());
        if (executable.isEmpty()) {
            qCWarning(DDEAMChecker) << "invalid TryExec value:" << tryExecVal.value().get();
            return false;
        }

        if (executable.startsWith(QDir::separator())) {
            const QFileInfo info{executable};
            return !(info.exists() && info.isExecutable());
        }
        return QStandardPaths::findExecutable(executable).isEmpty();
    }

    return false;
}

bool ApplicationFilter::showInCheck(const DesktopEntry &entry) noexcept
{
    const auto &desktops = getCurrentDesktops();
    if (desktops.isEmpty()) {
        return true;
    }

    bool showInCurrentDE{true};
    if (auto val = entry.value(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryOnlyShowIn)); val.has_value()) {
        showInCurrentDE = hasDesktopIntersection(toString(val.value()), desktops);
    }

    bool notShowInCurrentDE{false};
    if (auto val = entry.value(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryNotShowIn)); val.has_value()) {
        notShowInCurrentDE = hasDesktopIntersection(toString(val.value()), desktops);
    }

    return !showInCurrentDE || notShowInCurrentDE;
}
