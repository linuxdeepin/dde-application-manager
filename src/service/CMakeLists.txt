set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core DBus Concurrent Gui)
find_package(DtkCore REQUIRED)

pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb-icccm xcb-ewmh xcb)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0)
pkg_check_modules(GLib REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(QGSettings REQUIRED IMPORTED_TARGET gsettings-qt)

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

qt5_add_dbus_adaptor(ADAPTER_SOURCES
               ../../dbus/org.deepin.daemon.Mime.xml
               ../modules/mimeapp/mime_app.h
               MimeApp)


add_definitions(-DUSE_QT)

file(GLOB_RECURSE SRCS "*.h" "*.cpp" "../frameworkdbus/*.h" "../frameworkdbus/*.cpp"
    "../lib/*.h" "../lib/*.cpp" "../lib/*.hpp"
    "../modules/*.h" "../modules/*.cpp" "../modules/*.hpp"
    "../utils.h" "../utils/*.cpp")

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
    PkgConfig::GLib
    PkgConfig::QGSettings
    ${Qt5Gui_LIBRARIES}
)

target_include_directories(deepin-application-manager PUBLIC
    PkgConfig::XCB
    PkgConfig::X11
    PkgConfig::GIO
    PkgConfig::GLib
    PkgConfig::QGSettings
    ../lib
    ../utils
    ../frameworkdbus
    ../frameworkdbus/qtdbusextended
    ../frameworkdbus/types
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
)

# binary file
install(TARGETS deepin-application-manager DESTINATION bin)

add_definitions(-DQT_NO_KEYWORDS)
