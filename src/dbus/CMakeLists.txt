add_library(dde_am_dbus OBJECT)

file(
    GLOB_RECURSE dde_am_dbus_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

qt_add_dbus_adaptor(dde_am_dbus_SOURCE ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.xml dbus/applicationmanager1service.h ApplicationManager1Service)
qt_add_dbus_adaptor(dde_am_dbus_SOURCE ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.Application.xml dbus/applicationservice.h ApplicationService)
qt_add_dbus_adaptor(dde_am_dbus_SOURCE ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.Instance.xml dbus/instanceservice.h InstanceService)
qt_add_dbus_adaptor(dde_am_dbus_SOURCE ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.JobManager1.xml dbus/jobmanager1service.h JobManager1Service)
qt_add_dbus_adaptor(dde_am_dbus_SOURCE ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.JobManager1.Job.xml dbus/jobservice.h JobService)
qt_add_dbus_adaptor(dde_am_dbus_SOURCE ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ObjectManager1.xml dbus/applicationmanager1service.h ApplicationManager1Service AMobjectmanager1adaptor AMObjectManagerAdaptor)
qt_add_dbus_adaptor(dde_am_dbus_SOURCE ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ObjectManager1.xml dbus/applicationservice.h ApplicationService APPobjectmanager1adaptor APPObjectManagerAdaptor)
qt_add_dbus_adaptor(dde_am_dbus_SOURCE ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.MimeManager1.xml dbus/mimemanager1service.h MimeManager1Service)

if (NOT DEFINED TREELAND_PROTOCOLS_DATA_DIR)
    message(FATAL_ERROR "TREELAND_PROTOCOLS_DATA_DIR not set by TreelandProtocols package")
endif()

qt_generate_wayland_protocol_client_sources(dde_am_dbus
    FILES
        ${TREELAND_PROTOCOLS_DATA_DIR}/treeland-prelaunch-splash-v2.xml
)

target_sources(dde_am_dbus PRIVATE
    ${dde_am_dbus_SOURCE}
)

target_link_libraries(
    dde_am_dbus PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::WaylandClient
    Qt${QT_VERSION_MAJOR}::WaylandClientPrivate
    Dtk6::Core
    dde_am_utils
)

target_include_directories(
    dde_am_dbus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
)
