include(GNUInstallDirs)

set(BIN_NAME dde-application-manager)

set(DBUS_INTERFACE_DIR ${CMAKE_CURRENT_LIST_DIR}/dbus)

set(DBUSADAPTORS "")
file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_LIST_DIR}/*.cpp ${CMAKE_CURRENT_LIST_DIR}/*.h)

qt_add_dbus_adaptor(DBUSADAPTORS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.xml applicationmanager1service.h ApplicationManager1Service)
qt_add_dbus_adaptor(DBUSADAPTORS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.Application.xml applicationservice.h ApplicationService)
qt_add_dbus_adaptor(DBUSADAPTORS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.Instance.xml instanceservice.h InstanceService)
qt_add_dbus_adaptor(DBUSADAPTORS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.JobManager1.xml jobmanager1service.h JobManager1Service)
qt_add_dbus_adaptor(DBUSADAPTORS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.JobManager1.Job.xml jobservice.h JobService)

add_executable(${BIN_NAME} ${DBUSADAPTORS} ${SRCS})

target_include_directories(${BIN_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/dbus
    ${CMAKE_CURRENT_BINARY_DIR}/src
)

target_link_libraries(${BIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Concurrent
    Threads::Threads
)

install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
