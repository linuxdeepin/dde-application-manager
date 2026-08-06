find_package(Dtk6 REQUIRED COMPONENTS Core Tools)

dtk_add_config_to_cpp(
    AM_APP_OVERRIDE_CONFIG_SRC
    "${CMAKE_SOURCE_DIR}/misc/dsg/configs/dde-application-manager/org.deepin.dde.am.appoverride.json"
    OUTPUT_FILE_NAME "am_appoverride_config.hpp"
    CLASS_NAME "ApplicationOverrideConfig"
)

add_custom_target(am_appoverride_config_generate DEPENDS ${AM_APP_OVERRIDE_CONFIG_SRC})

add_subdirectory(dbus)

include(GNUInstallDirs)

configure_file(config.h.in config.h)

set(LIB_NAME dde_am_static)

file(GLOB SRCS ${CMAKE_CURRENT_LIST_DIR}/*.cpp ${CMAKE_CURRENT_LIST_DIR}/*.h)
list(REMOVE_ITEM SRCS ${UTILS_SRCS})

add_library(${LIB_NAME} STATIC ${SRCS} ${AM_APP_OVERRIDE_CONFIG_SRC})
add_dependencies(${LIB_NAME} am_appoverride_config_generate)

target_include_directories(${LIB_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

find_package(Dtk6 REQUIRED COMPONENTS Core)

target_link_libraries(${LIB_NAME} PUBLIC
    Threads::Threads
    dde_am_dbus
    Dtk6::Core
)

if (HAVE_DDE_API_EVENTLOGGER)
    target_compile_definitions(${LIB_NAME} PRIVATE HAVE_DDE_API_EVENTLOGGER)
    target_link_libraries(${LIB_NAME} PUBLIC DDEAPI::EventLogger)
endif()
