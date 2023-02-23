set(BIN_NAME dde-application-loader)

pkg_check_modules(LIBCAP REQUIRED IMPORTED_TARGET libcap)

set(SRCS
    ./main.cpp
    ../modules/tools/desktop_deconstruction.hpp
    ../modules/socket/client.h
    ../modules/socket/client.cpp
    ../modules/methods/task.hpp
    ../modules/methods/basic.h
    ../modules/methods/instance.hpp
    ../modules/methods/process_status.hpp
    ../modules/methods/registe.hpp
    ../modules/methods/use_mime_app_info.h
    ../modules/util/common.cpp
    ../modules/util/common.h
    ../modules/util/filesystem.cpp
    ../modules/util/filesystem.h
    ../modules/util/json.h
    ../modules/util/logger.cpp
    ../modules/util/logger.h
    ../modules/util/macro.h
    ../modules/util/oci_runtime.h
    ../modules/util/platform.cpp
    ../modules/util/platform.h
    ../modules/util/semaphore.cpp
    ../modules/util/semaphore.h
    ../modules/util/util.h
    ../modules/util/debug/debug.h
    ../modules/util/debug/debug.cpp
)

add_executable(${BIN_NAME} ${SRCS})

target_link_libraries(${BIN_NAME}
    pthread
    PkgConfig::LIBCAP
    stdc++fs
    Qt5::Core
)
install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
