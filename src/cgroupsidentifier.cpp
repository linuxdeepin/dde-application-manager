// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "cgroupsidentifier.h"
#include "global.h"
#include <QFile>
#include <QString>
#include <QDebug>

IdentifyRet CGroupsIdentifier::Identify(pid_t pid)
{
    auto AppCgroupPath = QString("/proc/%1/cgroup").arg(pid);
    QFile AppCgroupFile{AppCgroupPath};
    if (!AppCgroupFile.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text)) {
        qWarning() << "open " << AppCgroupPath << "failed: " << AppCgroupFile.errorString();
        return {};
    }
    auto UnitStr = parseCGroupsPath(QString::fromLocal8Bit(AppCgroupFile.readAll())
                                        .split(':', Qt::SkipEmptyParts)
                                        .last()
                                        .trimmed());  // FIXME: support CGroup version detection and multi-line parsing

    auto [appId, InstanceId] = processUnitName(UnitStr);
    return {std::move(appId), std::move(InstanceId)};
}

QString CGroupsIdentifier::parseCGroupsPath(const QString &CGP) noexcept
{
    if (CGP.isEmpty()) {
        qWarning() << "CGroupPath is empty.";
        return {};
    }
    auto CGPSlices = CGP.split('/', Qt::SkipEmptyParts);

    if (CGPSlices.first() != "user.slice") {
        qWarning() << "unrecognized process.";
        return {};
    }
    auto processUIDStr = CGPSlices[1].split('.').first().split('-').last();

    if (processUIDStr.isEmpty()) {
        qWarning() << "no uid found.";
        return {};
    }

    if (auto processUID = processUIDStr.toInt(); processUID != getCurrentUID()) {
        qWarning() << "process is not in CGroups of current user, ignore....";
        return {};
    }

    auto appInstance = CGPSlices.last();
    if (appInstance.isEmpty()) {
        qWarning() << "get AppId failed.";
        return {};
    }
    return appInstance;
}
