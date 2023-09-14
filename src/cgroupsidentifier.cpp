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

    auto UnitStr = parseCGroupsPath(AppCgroupFile);

    auto [appId, launcher, InstanceId] = processUnitName(UnitStr);
    return {std::move(appId), std::move(InstanceId)};
}

QString CGroupsIdentifier::parseCGroupsPath(QFile &cgroupFile) noexcept
{
    QTextStream stream{&cgroupFile};

    if (stream.atEnd()) {
        qWarning() << "read from cgroup file failed:" << stream.status();
        return {};
    }
    stream.setEncoding(QStringConverter::Utf8);
    QString CGP;
    while (!stream.atEnd()) {
        auto line = stream.readLine();
        auto firstColon = line.indexOf(':');
        auto secondColon = line.indexOf(':', firstColon + 1);
        auto subSystemd = QStringView(line.constBegin() + firstColon + 1, secondColon - firstColon - 1);
        if (subSystemd.isEmpty()) {  // cgroup v2
            CGP = line.last(line.size() - secondColon - 1);
            break;
        }

        if (subSystemd == QString{"name=systemd"}) {         // cgroup v1
            CGP = line.last(line.size() - secondColon - 1);  // shouldn't break, maybe v1 and v2 exists at the same time.
        }
    }

    if (CGP.isEmpty()) {
        qWarning() << "no systemd informations found.";
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
