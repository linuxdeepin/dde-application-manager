// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"
#include <QDBusConnection>
#include <QCoreApplication>
#include <QDir>
#include "dbus/applicationmanager1service.h"
#include "cgroupsidentifier.h"
#include <chrono>
#include <iostream>

Q_LOGGING_CATEGORY(DDEAMProf, "dde.am.prof", QtInfoMsg)

namespace {
void registerComplexDbusType()
{
    qDBusRegisterMetaType<QMap<QString, QDBusUnixFileDescriptor>>();
    qDBusRegisterMetaType<QMap<uint, QMap<QString, QDBusUnixFileDescriptor>>>();
    qDBusRegisterMetaType<IconMap>();
    qDBusRegisterMetaType<ObjectMap>();
}
}  // namespace

int main(int argc, char *argv[])
{
    auto start = std::chrono::high_resolution_clock::now();
    QCoreApplication app{argc, argv};

    auto &bus = ApplicationManager1DBus::instance();
    bus.initGlobalServerBus(DBusType::Session);
    bus.setDestBus();
    auto &AMBus = bus.globalServerBus();

    registerComplexDbusType();
    ApplicationManager1Service AMService{std::make_unique<CGroupsIdentifier>(), AMBus};
    QList<DesktopFile> fileList{};
    auto desktopFileDirs = getXDGDataDirs();

    applyIteratively(QList<QDir>(desktopFileDirs.begin(), desktopFileDirs.end()), [&AMService](const QFileInfo &info) -> bool {
        DesktopErrorCode err{DesktopErrorCode::NoError};
        auto ret = DesktopFile::searchDesktopFileByPath(info.absoluteFilePath(), err);
        if (!ret.has_value()) {
            qWarning() << "failed to search File:" << err;
            return false;
        }
        AMService.addApplication(std::move(ret).value());
        return false;  // means to apply this function to the rest of the files
    });
    auto end = std::chrono::high_resolution_clock::now();
    qCInfo(DDEAMProf) << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms";
    return
#ifdef PROFILING_MODE
        QCoreApplication::exec();
#else
        0;
#endif
}
