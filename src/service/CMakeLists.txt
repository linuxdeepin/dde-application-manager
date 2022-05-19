set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core DBus Concurrent)
find_package(DtkCore REQUIRED)

pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb-icccm xcb-ewmh xcb)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../dbus/org.desktopspec.ApplicationManager.xml
                     impl/application_manager.h
                     ApplicationManager)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../dbus/org.desktopspec.Application.xml
                     impl/application.h
                     Application)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../dbus/org.desktopspec.ApplicationInstance.xml
                     impl/application_instance.h
                     ApplicationInstance)

add_definitions(-DUSE_QT)

set(SRCS ${PROJECT_BINARY_DIR}/src/define.h)

set(SRCS
    ./applicationhelper.h
    ./main.cpp
    ./impl/application_manager.h
    ./impl/application_manager.cpp
    ./impl/application.h
    ./impl/application.cpp
    ./impl/application_instance.h
    ./impl/application_instance.cpp
    ../modules/tools/desktop_deconstruction.hpp
    ../modules/socket/server.h
    ../modules/socket/server.cpp
    ../modules/methods/basic.h
    ../modules/methods/task.hpp
    ../modules/methods/instance.hpp
    ../modules/methods/quit.hpp
    ../modules/methods/registe.hpp
    ../lib/desktopinfo.h
    ../lib/desktopinfo.cpp
    ../lib/dlocale.h
    ../lib/dlocale.cpp
    ../lib/dstring.h
    ../lib/dstring.cpp
    ../lib/keyfile.h
    ../lib/keyfile.cpp
    ../lib/lang.h
    ../lib/macro.h
    ../lib/process.h
    ../lib/process.cpp
    ../lib/dfile.h
    ../lib/dfile.cpp
    ../lib/basedir.h
    ../lib/basedir.cpp
    ../lib/xcbutils.h
    ../lib/xcbutils.cpp
    ../lib/applaunchcontext.h
    ../lib/applaunchcontext.cpp
    ../lib/meminfo.h
    ../lib/meminfo.cpp
    ../lib/gsetting.h
    ../lib/gsetting.cpp
    ../utils/settings.h
    ../utils/settings.cpp
    ../utils/synmodulebase.h
    ../utils/synmodule.h
    ../utils/synconfig.h
    ../utils/synconfig.cpp
    ../modules/apps/alrecorder.h
    ../modules/apps/alrecorder.cpp
    ../modules/apps/appmanager.h
    ../modules/apps/appmanager.cpp
    ../modules/apps/dfwatcher.h
    ../modules/apps/dfwatcher.cpp
    ../modules/apps/dbusalrecorderadaptor.h
    ../modules/apps/dbusalrecorderadaptor.cpp
    ../modules/launcher/common.h
    ../modules/launcher/launcher.h
    ../modules/launcher/launcher.cpp
    ../modules/launcher/launchermanager.h
    ../modules/launcher/launchermanager.cpp
    ../modules/launcher/category.h
    ../modules/launcher/category.cpp
    ../modules/launcher/launchersettings.h
    ../modules/launcher/launchersettings.cpp
    ../modules/launcher/dbusadaptorlauncher.h
    ../modules/launcher/dbusadaptorlauncher.cpp
    ../modules/dock/dock.h
    ../modules/dock/dock.cpp
    ../modules/dock/dockmanager.h
    ../modules/dock/dockmanager.cpp
    ../modules/dock/windowidentify.h
    ../modules/dock/windowidentify.cpp
    ../modules/dock/windowinfobase.h
    ../modules/dock/windowinfox.h
    ../modules/dock/windowinfox.cpp
    ../modules/dock/windowinfok.h
    ../modules/dock/windowinfok.cpp
    ../modules/dock/dbushandler.h
    ../modules/dock/dbushandler.cpp
    ../modules/dock/docksettings.h
    ../modules/dock/docksettings.cpp
    ../modules/dock/common.h
    ../modules/dock/appinfo.h
    ../modules/dock/appinfo.cpp
    ../modules/dock/entry.h
    ../modules/dock/entry.cpp
    ../modules/dock/entries.h
    ../modules/dock/entries.cpp
    ../modules/dock/appmenu.h
    ../modules/dock/appmenu.cpp
    ../modules/dock/processinfo.h
    ../modules/dock/processinfo.cpp
    ../modules/dock/windowpatterns.h
    ../modules/dock/windowpatterns.cpp
    ../modules/dock/waylandmanager.h
    ../modules/dock/waylandmanager.cpp
    ../modules/dock/x11manager.h
    ../modules/dock/x11manager.cpp
    ../modules/dock/dbusadaptordock.h
    ../modules/dock/dbusadaptordock.cpp
    ../modules/dock/dbusadaptorentry.h
    ../modules/dock/dbusadaptorentry.cpp
    ../modules/startmanager/common.h
    ../modules/startmanager/startmanager.h
    ../modules/startmanager/startmanager.cpp
    ../modules/startmanager/startmanagersettings.h
    ../modules/startmanager/startmanagersettings.cpp
    ../modules/startmanager/startmanagerdbushandler.h
    ../modules/startmanager/startmanagerdbushandler.cpp
    ../frameworkdbus/types/launcheriteminfo.h
    ../frameworkdbus/types/launcheriteminfo.cpp
    ../frameworkdbus/types/launcheriteminfolist.h
    ../frameworkdbus/types/launcheriteminfolist.cpp
    ../frameworkdbus/types/rect.h
    ../frameworkdbus/types/rect.cpp
    ../frameworkdbus/types/unlaunchedappmap.h
    ../frameworkdbus/types/unlaunchedappmap.cpp
    ../frameworkdbus/dbuslauncherfront.h
    ../frameworkdbus/dbuslauncherfront.cpp
    ../frameworkdbus/dbuslauncher.h
    ../frameworkdbus/dbuslauncher.cpp
    ../frameworkdbus/dbuswm.h
    ../frameworkdbus/dbuswm.cpp
    ../frameworkdbus/dbuswmswitcher.h
    ../frameworkdbus/dbuswmswitcher.cpp
    ../frameworkdbus/dbuskwaylandwindowmanager.h
    ../frameworkdbus/dbuskwaylandwindowmanager.cpp
    ../frameworkdbus/dbusplasmawindow.h
    ../frameworkdbus/dbusplasmawindow.cpp
    ../frameworkdbus/dbuskwaylandoutput.h
    ../frameworkdbus/dbuskwaylandoutput.cpp
    ../frameworkdbus/dbusbamfapplication.h
    ../frameworkdbus/dbusbamfapplication.cpp
    ../frameworkdbus/dbusbamfmatcher.h
    ../frameworkdbus/dbusbamfmatcher.cpp
    ../frameworkdbus/qtdbusextended/dbusextended.h
    ../frameworkdbus/qtdbusextended/dbusextendedabstractinterface.h
    ../frameworkdbus/qtdbusextended/dbusextendedpendingcallwatcher_p.h
    ../frameworkdbus/qtdbusextended/dbusextendedabstractinterface.cpp
    ../frameworkdbus/qtdbusextended/dbusextendedpendingcallwatcher.cpp
    ../frameworkdbus/qtdbusextended/DBusExtended
    ../frameworkdbus/qtdbusextended/DBusExtendedAbstractInterface
)

add_executable(deepin-application-manager
    ${SRCS}
    ${ADAPTER_SOURCES}
)

target_link_libraries(deepin-application-manager
    Qt5::Core
    Qt5::DBus
    Qt5::Concurrent
    ${DtkCore_LIBRARIES}
    pthread
    PkgConfig::XCB
    PkgConfig::X11
    PkgConfig::GIO
)

target_include_directories(deepin-application-manager PUBLIC
    PkgConfig::XCB
    PkgConfig::GIO
    ../lib
    ../utils
    ../frameworkdbus
    ../frameworkdbus/qtdbusextended
    ../frameworkdbus/types
)

install(TARGETS deepin-application-manager DESTINATION bin)
