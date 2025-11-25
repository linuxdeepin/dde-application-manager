add_subdirectory(dbus)

include(GNUInstallDirs)

configure_file(config.h.in config.h)

set(LIB_NAME dde_am_static)
set(UTILS_NAME dde_am_utils)

set(UTILS_SRCS
    applicationmanagerstorage.cpp
    applicationmanagerstorage.h
    desktopentry.cpp
    desktopentry.h
    desktopfileparser.cpp
    desktopfileparser.h
    global.h
    constant.h
    iniParser.h
    config.h
    applicationchecker.cpp
    applicationchecker.h
)

add_library(${UTILS_NAME} OBJECT ${UTILS_SRCS})

target_include_directories(${UTILS_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${UTILS_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
)

file(GLOB SRCS ${CMAKE_CURRENT_LIST_DIR}/*.cpp ${CMAKE_CURRENT_LIST_DIR}/*.h)
list(REMOVE_ITEM SRCS ${UTILS_SRCS})

add_library(${LIB_NAME} STATIC ${SRCS})

target_include_directories(${LIB_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${LIB_NAME} PUBLIC
    Threads::Threads
    dde_am_dbus
    ${UTILS_NAME}
)
