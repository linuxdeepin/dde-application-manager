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

set(BUILD_EXAMPLES ON CACHE BOOL "Whether to build examples or not.")

find_package(Qt6 REQUIRED COMPONENTS Core DBus Concurrent)
find_package(Threads REQUIRED)

set(APP_LAUNCH_HELPER_BIN app-launch-helper)

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(apps)

include(CTest)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
