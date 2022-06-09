set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core DBus Concurrent Gui)
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
    ${Qt5Gui_LIBRARIES}
)

target_include_directories(deepin-application-manager PUBLIC
    PkgConfig::XCB
    PkgConfig::GIO
    ../lib
    ../utils
    ../frameworkdbus
    ../frameworkdbus/qtdbusextended
    ../frameworkdbus/types
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
)

# binary file
install(TARGETS deepin-application-manager DESTINATION bin)

