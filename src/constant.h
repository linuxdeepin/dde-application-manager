// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef CONSTANT_H
#define CONSTANT_H

#include "common/constant.h"
#include <QStringView>

constexpr static auto &SystemdPropInterfaceName = u"org.freedesktop.DBus.Properties";
constexpr static auto &SystemdUnitInterfaceName = u"org.freedesktop.systemd1.Unit";
constexpr static auto &DDEApplicationManager1ServiceName =
#ifdef DDE_AM_USE_DEBUG_DBUS_NAME
    u"org.desktopspec.debug.ApplicationManager1";
#else
    u"org.desktopspec.ApplicationManager1";
#endif

constexpr static auto &DDEApplicationManager1ObjectPath = u"/org/desktopspec/ApplicationManager1";
constexpr static auto &DDEApplicationManager1JobManager1ObjectPath = u"/org/desktopspec/ApplicationManager1/JobManager1";
constexpr static auto &DDEApplicationManager1MimeManager1ObjectPath = u"/org/desktopspec/ApplicationManager1/MimeManager1";
constexpr static auto &DesktopFileEntryKey = u"Desktop Entry";
constexpr static auto &DesktopFileActionKey = u"Desktop Action ";
constexpr static auto &DesktopFileDefaultKeyLocale = u"default";
constexpr static auto &X_Deepin_GenerateSource = u"X-Deepin-GenerateSource";
constexpr static auto &X_Deepin_Singleton = u"X-Deepin-Singleton";
constexpr static auto &DesktopEntryHidden = u"Hidden";
constexpr static auto &DesktopEntryExec = u"Exec";
constexpr static auto &DesktopEntryEnv = u"Env";
constexpr static auto &DesktopDefault = u"default";

constexpr static auto &ApplicationManagerServerDBusName =
#ifdef DDE_AM_USE_DEBUG_DBUS_NAME
    u"deepin_application_manager_debug_server_bus";
#else
    u"deepin_application_manager_server_bus";
#endif

constexpr static auto &ApplicationManagerDestDBusName =
#ifdef DDE_AM_USE_DEBUG_DBUS_NAME
    u"deepin_application_manager_debug_dest_bus";
#else
    u"deepin_application_manager_dest_bus";
#endif

constexpr static auto &ObjectManagerInterface = u"org.desktopspec.DBus.ObjectManager";
constexpr static auto &JobManager1Interface = u"org.desktopspec.JobManager1";
constexpr static auto &JobInterface = u"org.desktopspec.JobManager1.Job";
constexpr static auto &ApplicationManager1Interface = u"org.desktopspec.ApplicationManager1";
constexpr static auto &InstanceInterface = u"org.desktopspec.ApplicationManager1.Instance";
constexpr static auto &ApplicationInterface = u"org.desktopspec.ApplicationManager1.Application";
constexpr static auto &MimeManager1Interface = u"org.desktopspec.MimeManager1";

constexpr static auto &systemdOption = u"systemd";
constexpr static auto &splitOption = u"split";
constexpr static auto &AppExecOption = u"appExec";

constexpr static auto STORAGE_VERSION = 0;
constexpr static auto &ApplicationPropertiesGroup = u"Application Properties";
constexpr static auto & LastLaunchedTime = u"LastLaunchedTime";
constexpr static auto & Environ = u"Environ";
constexpr static auto & LaunchedTimes = u"LaunchedTimes";
constexpr static auto & InstalledTime = u"InstalledTime";

constexpr static auto & ApplicationManagerHookDir = u"/deepin/dde-application-manager/hooks.d";

constexpr static auto & ApplicationManagerToolsConfig = u"org.deepin.dde.am";

constexpr static auto & ApplicationManagerConfig = u"org.deepin.dde.application-manager";
constexpr static auto & AppExtraEnvironments = u"appExtraEnvironments";
constexpr static auto & AppEnvironmentsBlacklist = u"appEnvironmentsBlacklist";

constexpr static auto & CompatibilityConfigFilePath = u"/var/lib/compatible/compatibleDesktop.json";

constexpr auto desktopSuffix = QStringView{u".desktop"};

#endif
