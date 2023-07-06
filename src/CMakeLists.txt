include(GNUInstallDirs)

set(BIN_NAME dde-application-manager)

set(DBUS_INTERFACE_DIR ${CMAKE_CURRENT_LIST_DIR}/dbus)

function(generation_dbus_interface xml class_name class_file)
    execute_process(COMMAND ${DTK_XML2CPP} -c ${class_name} -p ${class_file} ${xml}
        WORKING_DIRECTORY ${DBUS_INTERFACE_DIR})
endfunction(generation_dbus_interface)

set(DBUS "")
file(GLOB SRCS ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_LIST_DIR}/include/*)

qt_add_dbus_interface(DBUS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.xml ApplicationManager)
qt_add_dbus_interface(DBUS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.Application.xml Application)
qt_add_dbus_interface(DBUS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.Instance.xml Instance)
qt_add_dbus_interface(DBUS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.JobManager1.xml JobManager)
qt_add_dbus_interface(DBUS ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.JobManager1.Job.xml Job)

add_executable(${BIN_NAME} ${DBUS} ${SRCS})

target_include_directories(${BIN_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${BIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Threads::Threads
)

install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
