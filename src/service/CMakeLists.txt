set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../DBus/org.desktopspec.ApplicationManager.xml
                     impl/application_manager.h
                     ApplicationManager)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../DBus/org.desktopspec.Application.xml
                     impl/application.h
                     Application)

qt5_add_dbus_adaptor(ADAPTER_SOURCES
                     ../../DBus/org.desktopspec.ApplicationInstance.xml
                     impl/application_instance.h
                     ApplicationInstance)
add_definitions(-DUSE_QT)

set(SRCS ${PROJECT_BINARY_DIR}/src/define.h)

set(SRCS
    ./main.cpp
    ./impl/application_manager.h
    ./impl/application_manager.cpp
    ./impl/application.h
    ./impl/application.cpp
    ./impl/application_instance.h
    ./impl/application_instance.cpp
    ../modules/applicationhelper/helper.h
    ../modules/tools/desktop_deconstruction.hpp
    ../modules/socket/server.h
    ../modules/socket/server.cpp
    ../modules/methods/basic.h
    ../modules/methods/task.hpp
    ../modules/methods/instance.hpp
    ../modules/methods/quit.hpp
    ../modules/methods/registe.hpp
)

add_executable(deepin-application-manager
    ${SRCS}
    ${ADAPTER_SOURCES}
)

target_link_libraries(deepin-application-manager
    Qt5::Core
    Qt5::DBus
    Qt5::Concurrent
)

install(TARGETS deepin-application-manager DESTINATION bin)
