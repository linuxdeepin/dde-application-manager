include(FindPkgConfig)
find_package(PkgConfig REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEFINE_LOADER_PATH)
  set(LOADER_PATH ${PROJECT_BINARY_DIR}/src/loader/deepin-application-loader)
endif()

configure_file(
        "${PROJECT_SOURCE_DIR}/src/define.h.in"
        "${PROJECT_BINARY_DIR}/src/define.h"
)

add_subdirectory("service")
add_subdirectory("loader")
