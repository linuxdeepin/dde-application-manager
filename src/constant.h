// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef CONSTANT_H
#define CONSTANT_H

constexpr auto SystemdService = u8"org.freedesktop.systemd1";
constexpr auto SystemdObjectPath = u8"/org/freedesktop/systemd1";
constexpr auto SystemdInterfaceName = u8"org.freedesktop.systemd1.Manager";
constexpr auto DDEApplicationManager1ServiceName =
#ifdef DEBUG_MODE
    u8"org.deepin.dde.debug.ApplicationManager1";
#else
    u8"org.deepin.dde.ApplicationManager1";
#endif

constexpr auto DDEApplicationManager1ObjectPath = u8"/org/deepin/dde/ApplicationManager1";
constexpr auto DDEAutoStartManager1ObjectPath = u8"/org/deepin/dde/AutoStartManager1";
constexpr auto DDEApplicationManager1JobManagerObjectPath = u8"/org/deepin/dde/ApplicationManager1/JobManager1";
constexpr auto DesktopFileEntryKey = u8"Desktop Entry";
constexpr auto DesktopFileActionKey = u8"Desktop Action ";

constexpr auto ApplicationManagerServerDBusName =
#ifdef DEBUG_MODE
    u8"deepin_application_manager_debug_server_bus";
#else
    u8"deepin_application_manager_server_bus";
#endif

constexpr auto ApplicationManagerDestDBusName =
#ifdef DEBUG_MODE
    u8"deepin_application_manager_debug_dest_bus";
#else
    u8"deepin_application_manager_dest_bus";
#endif

constexpr auto ObjectManagerInterface = "org.desktopspec.DBus.ObjectManager";
constexpr auto JobManagerInterface = "org.desktopspec.JobManager1";
constexpr auto JobInterface = "org.desktopspec.JobManager1.Job";
constexpr auto ApplicationManagerInterface = "org.desktopspec.ApplicationManager1";
constexpr auto InstanceInterface = "org.desktopspec.ApplicationManager1.Instance";
constexpr auto ApplicationInterface = "org.desktopspec.ApplicationManager1.Application";

#endif
