cmake_minimum_required(VERSION 3.11)

project(deepin-application-manager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(misc)

if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    # generate qm
    execute_process(COMMAND bash "translate_generation.sh"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif ()

# qm files
file(GLOB QM_FILES "translations/*.qm")
install(FILES ${QM_FILES}
    DESTINATION share/dde-application-manager/translations)
