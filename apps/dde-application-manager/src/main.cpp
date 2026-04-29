// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationmanagerstorage.h"
#include "cgroupsidentifier.h"
#include "dbus/applicationmanager1service.h"
#include "global.h"
#include <QDBusConnection>
#include <QCoreApplication>

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
    using namespace Qt::StringLiterals;
    setenv("DSG_APP_ID", fromStaticRaw(ApplicationManagerConfig).toUtf8().constData(), 0);
#ifdef PROFILING_MODE
    auto start = std::chrono::high_resolution_clock::now();
#endif
    const QCoreApplication app{argc, argv};

    auto &bus = ApplicationManager1DBus::instance();
    bus.initGlobalServerBus(DBusType::Session);
    bus.setDestBus();

    registerComplexDbusType();
    auto storageDir = getXDGDataHome() % u"/deepin/ApplicationManager"_s;
    auto storage = ApplicationManager1Storage::createApplicationManager1Storage(storageDir);

    ApplicationManager1Service AMService{std::make_unique<CGroupsIdentifier>(), storage};
    AMService.initService(bus.globalServerBus());

#ifdef PROFILING_MODE
    auto end = std::chrono::high_resolution_clock::now();
    qCInfo(DDEAMProf) << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms";
    return 0;
#else
    return QCoreApplication::exec();
#endif
}
