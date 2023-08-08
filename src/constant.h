// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef CONSTANT_H
#define CONSTANT_H

constexpr auto SystemdService = u8"org.freedesktop.systemd1";
constexpr auto SystemdObjectPath = u8"/org/freedesktop/systemd1";
constexpr auto SystemdInterfaceName = u8"org.freedesktop.systemd1.Manager";
constexpr auto DDEApplicationManager1ServiceName = u8"org.deepin.dde.ApplicationManager1";
constexpr auto DDEApplicationManager1ObjectPath = u8"/org/deepin/dde/ApplicationManager1";
constexpr auto DDEApplicationManager1ApplicationObjectPath = u8"/org/deepin/dde/ApplicationManager1/Application/";
constexpr auto DDEApplicationManager1InstanceObjectPath = u8"/org/deepin/dde/ApplicationManager1/Instance/";
constexpr auto DDEApplicationManager1JobManagerObjectPath = u8"/org/deepin/dde/ApplicationManager1/JobManager1";
constexpr auto DDEApplicationManager1JobObjectPath = u8"/org/deepin/dde/ApplicationManager1/JobManager1/Job/";
constexpr auto DesktopFileEntryKey = u8"Desktop Entry";
constexpr auto DesktopFileActionKey = u8"Desktop Action ";
constexpr auto ApplicationManagerServerDBusName = u8"deepin_application_manager_server_bus";
constexpr auto ApplicationManagerDestDBusName = "deepin_application_manager_dest_bus";

#endif
