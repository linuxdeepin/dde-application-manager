cmake_minimum_required(VERSION 3.20)
project(dde-session-shell
    VERSION 0.1.0
    DESCRIPTION "deepin applicaion manager"
    HOMEPAGE_URL "https://github.com/linuxdeepin/dde-application-manager"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Concurrent)
find_package(Threads REQUIRED)

qt_add_dbus_adaptor(ApplicationManagerSource ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.xml applicationmanager1service.h ApplicationManager1Service)
add_library(ApplicationManager OBJECT ${ApplicationManagerSource})
qt_add_dbus_adaptor(ApplicationSource ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.Application.xml applicationservice.h ApplicationService)
add_library(Application OBJECT ${ApplicationSource})
qt_add_dbus_adaptor(InstnaceSource ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.ApplicationManager1.Instance.xml instanceservice.h InstanceService)
add_library(Instance OBJECT ${InstnaceSource})
qt_add_dbus_adaptor(JobManagerSource ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.JobManager1.xml jobmanager1service.h JobManager1Service)
add_library(JobManager OBJECT ${JobManagerSource})
qt_add_dbus_adaptor(JobSource ${PROJECT_SOURCE_DIR}/api/dbus/org.desktopspec.JobManager1.Job.xml jobservice.h JobService)
add_library(Job OBJECT ${JobSource})

set(AdaptorLib)
list(APPEND AdaptorLib ApplicationManager Application Instance JobManager Job)

foreach(obj IN LISTS AdaptorLib)
    target_link_libraries(${obj} PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::DBus
        Qt${QT_VERSION_MAJOR}::Concurrent
    )

    target_include_directories(${obj} PUBLIC
        ${PROJECT_SOURCE_DIR}/src/include
        ${PROJECT_SOURCE_DIR}/src/dbus
    )
endforeach()

add_subdirectory(src)
add_subdirectory(plugin)

# add_subdirectory(docs)
include(CTest)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
