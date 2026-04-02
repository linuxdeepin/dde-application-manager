// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "cgroupsidentifier.h"
#include "global.h"
#include <QFile>
#include <QString>
#include <QDebug>

IdentifyRet CGroupsIdentifier::Identify(const QDBusUnixFileDescriptor &pidfd)
{
    // Extract PID from pidfd
    auto pid = getPidFromPidFd(pidfd);
    if (pid == 0) {
        qWarning() << "Failed to extract PID from pidfd";
        return {};
    }

    using namespace Qt::StringLiterals;
    // Perform identification using PID
    auto AppCgroupPath = u"/proc/" % QString::number(pid) % u"/cgroup";
    QFile AppCgroupFile{AppCgroupPath};
    if (!AppCgroupFile.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text)) {
        qWarning() << "open " << AppCgroupPath << "failed: " << AppCgroupFile.errorString();
        return {};
    }

    auto UnitStr = parseCGroupsPath(AppCgroupFile);

    if (UnitStr.isEmpty()) {
        qWarning() << "process CGroups file failed.";
        return {};
    }

    auto value = processUnitName(UnitStr);
    if (!value) {
        qWarning() << "processUnitName failed.";
        return {};
    }

    auto [appId, launcher, InstanceId] = std::move(value).value();

    // Verify that the pidfd still refers to the same process to avoid timing issues
    // where the process exits and the PID is reused by another process
    if (pidfd_send_signal(pidfd.fileDescriptor(), 0, nullptr, 0) != 0) {
        const int errorCode = errno;
        qWarning() << "pidfd_send_signal failed with errno:" << errorCode << ", description:" << strerror(errorCode);
        qWarning() << "pidfd is no longer valid (process may have exited)";
        return {};
    }

    return {std::move(appId), std::move(InstanceId)};
}

QString CGroupsIdentifier::parseCGroupsPath(QFile &cgroupFile) noexcept
{
    const auto data = QString::fromUtf8(cgroupFile.readAll());
    if (data.isEmpty()) {
        return {};
    }

    const QStringView contentView{data};

    QStringView v2Path;
    QStringView v1Path;

    auto lines = qTokenize(contentView, u'\n', Qt::SkipEmptyParts);
    for (auto line : lines) {
        auto trimmed = line.trimmed();

        auto fields = qTokenize(trimmed, u':');
        auto it = fields.begin();
        const auto end = fields.end();

        // Index 0: Hierarchy ID (ignore)
        if (it == end) {
            continue;
        }

        // Index 1: Subsystems
        if (++it == end) {
            continue;
        }
        const auto subsystems = *it;

        // Index 2: Path
        if (++it == end) {
            continue;
        }
        const auto path = *it;

        // v2 first
        if (subsystems.isEmpty()) {
            v2Path = path;
            break;
        }

        if (subsystems == u"name=systemd") {
            v1Path = path;  // v1 second
        }
    }

    const QStringView targetPath = !v2Path.isEmpty() ? v2Path : v1Path;
    if (targetPath.isEmpty()) {
        return {};
    }

    auto tokens = qTokenize(targetPath, u'/', Qt::SkipEmptyParts);
    auto pit = tokens.begin();
    const auto pend = tokens.end();

    if (pit == pend || *pit != u"user.slice") {
        return {};
    }

    if (++pit == pend) {
        return {};
    }
    const auto userSlice = *pit;

    if (!userSlice.startsWith(u"user-") || !userSlice.endsWith(u".slice")) {
        return {};
    }

    const auto uidPart = userSlice.sliced(5, userSlice.size() - 5 - 6);
    bool ok{false};
    if (uidPart.toUInt(&ok) != getCurrentUID() || !ok) {
        return {};
    }

    auto lastToken = userSlice;
    while (++pit != pend) {
        lastToken = *pit;
    }

    return lastToken.toString();
}
