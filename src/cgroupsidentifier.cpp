// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "cgroupsidentifier.h"
#include "global.h"
#include <QFile>
#include <QString>
#include <QDebug>

IdentifyRet CGroupsIdentifier::Identify(const QDBusUnixFileDescriptor &pidfd)
{
    // Get cgroup information using new approach
    auto [cgroupInode, cgroupPath] = getCGroupInfoFromPidFd(pidfd);
    
    if (cgroupPath.isEmpty()) {
        qWarning() << "Failed to get cgroup information from pidfd";
        return {};
    }

    auto cgroupSlices = cgroupPath.split('/', Qt::SkipEmptyParts);
    if (cgroupSlices.isEmpty()) {
        qCritical() << "invalid cgroups path, abort.";
        return {};
    }

    if (cgroupSlices.first() != "user.slice") {
        qWarning() << "unrecognized process.";
        return {};
    }

    if (cgroupSlices.size() < 2) {
        qWarning() << "couldn't detect uid.";
        return {};
    }
    
    auto userSlice = cgroupSlices.at(1);
    if (userSlice.isEmpty()) {
        qWarning() << "couldn't detect uid.";
        return {};
    }
    auto processUIDStr = userSlice.split('.').first().split('-').last();

    if (processUIDStr.isEmpty()) {
        qWarning() << "no uid found.";
        return {};
    }

    if (auto processUID = processUIDStr.toInt(); static_cast<uid_t>(processUID) != getCurrentUID()) {
        qWarning() << "process is not in CGroups of current user, ignore....";
        return {};
    }

    auto appInstance = cgroupSlices.last();
    if (appInstance.isEmpty()) {
        qWarning() << "get AppId failed.";
        return {};
    }
    
    auto [appId, launcher, instanceId] = processUnitName(appInstance);
    
    // For kernels without PIDFD_GET_INFO support, perform comprehensive validation
    // at the end to ensure pidfd is still valid when we return the result
    if (cgroupInode == 0) {
        // Verify that the pidfd still refers to the same process
        if (pidfd_send_signal(pidfd.fileDescriptor(), 0, nullptr, 0) != 0) {
            const int errorCode = errno;
            qWarning() << "pidfd_send_signal failed with errno:" << errorCode << ", description:" << strerror(errorCode);
            qWarning() << "pidfd is no longer valid (process may have exited)";
            return {};
        }
    }
    
    return {std::move(appId), std::move(instanceId)};
}
