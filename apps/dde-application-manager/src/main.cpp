// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationmanagerstorage.h"
#include "cgroupsidentifier.h"
#include "dbus/applicationmanager1service.h"
#include "global.h"
#include <QDBusConnection>
#include <QGuiApplication>

Q_LOGGING_CATEGORY(DDEAMProf, "dde.am.prof", QtInfoMsg)

namespace {
void registerComplexDbusType()
{
    qRegisterMetaType<ObjectInterfaceMap>();
    qDBusRegisterMetaType<ObjectInterfaceMap>();
    qRegisterMetaType<ObjectMap>();
    qDBusRegisterMetaType<ObjectMap>();
    qDBusRegisterMetaType<QStringMap>();
    qRegisterMetaType<QStringMap>();
    qRegisterMetaType<PropMap>();
    qDBusRegisterMetaType<PropMap>();
    qDBusRegisterMetaType<QDBusObjectPath>();
    qDBusRegisterMetaType<SystemdExecCommand>();
    qDBusRegisterMetaType<QList<SystemdExecCommand>>();
    qDBusRegisterMetaType<SystemdProperty>();
    qDBusRegisterMetaType<QList<SystemdProperty>>();
    qDBusRegisterMetaType<SystemdAux>();
    qDBusRegisterMetaType<QList<SystemdAux>>();
}
}  // namespace

int main(int argc, char *argv[])
{
    setenv("DSG_APP_ID", ApplicationManagerConfig, 0);
#ifdef PROFILING_MODE
    auto start = std::chrono::high_resolution_clock::now();
#endif
    QGuiApplication app{argc, argv};

    auto &bus = ApplicationManager1DBus::instance();
    bus.initGlobalServerBus(DBusType::Session);
    bus.setDestBus();
    auto &AMBus = bus.globalServerBus();

    registerComplexDbusType();
    auto storageDir = getXDGDataHome() + QDir::separator() + "deepin" + QDir::separator() + "ApplicationManager";
    auto storage = ApplicationManager1Storage::createApplicationManager1Storage(storageDir);

    ApplicationManager1Service AMService{std::make_unique<CGroupsIdentifier>(), storage};
    AMService.initService(AMBus);

#ifdef PROFILING_MODE
    auto end = std::chrono::high_resolution_clock::now();
    qCInfo(DDEAMProf) << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms";
    return 0;
#else
    return QGuiApplication::exec();
#endif
}
