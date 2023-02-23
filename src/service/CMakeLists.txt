set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(BIN_NAME dde-application-manager)

find_package(PkgConfig REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core DBus Concurrent Gui)
find_package(DtkCore REQUIRED)
find_package(DtkWidget REQUIRED)

pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb-icccm xcb-ewmh xcb)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0)
pkg_check_modules(GLib REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(QGSettings REQUIRED IMPORTED_TARGET gsettings-qt)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../dbus/org.deepin.dde.Application1.Manager.xml
                     impl/application_manager.h
                     ApplicationManager)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../dbus/org.deepin.dde.Application1.xml
                     impl/application.h
                     Application)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../dbus/org.deepin.dde.Application1.Instance.xml
                     impl/application_instance.h
                     ApplicationInstance)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
               ../../dbus/org.deepin.dde.Mime1.xml
               ../modules/mimeapp/mime_app.h
               MimeApp)


add_definitions(-DUSE_QT)

file(GLOB_RECURSE SRCS "*.h" "*.cpp" "../frameworkdbus/*.h" "../frameworkdbus/*.cpp"
    "../lib/*.h" "../lib/*.cpp" "../lib/*.hpp"
    "../modules/*.h" "../modules/*.cpp" "../modules/*.hpp"
    "../utils.h" "../utils/*.cpp")

add_executable(${BIN_NAME}
    ${SRCS}
    ${ADAPTER_SOURCES}
)

target_link_libraries(${BIN_NAME}
    Qt5::Core
    Qt5::DBus
    Qt5::Concurrent
    Qt5::Gui
    Dtk::Core
    pthread
    PkgConfig::XCB
    PkgConfig::X11
    PkgConfig::GIO
    PkgConfig::GLib
    PkgConfig::QGSettings
    ${DtkWidget_LIBRARIES}
)

target_include_directories(${BIN_NAME} PUBLIC
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
install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

add_definitions(-DQT_NO_KEYWORDS)
