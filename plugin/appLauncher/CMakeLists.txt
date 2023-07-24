include(GNUInstallDirs)

set(APP_LAUNCHER_BIN_NAME am-launcher)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

add_executable(${APP_LAUNCHER_BIN_NAME} launcher.cpp)
target_link_libraries(${APP_LAUNCHER_BIN_NAME} PRIVATE
    PkgConfig::SYSTEMD
)

install(TARGETS ${APP_LAUNCHER_BIN_NAME} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/deepin/application-manager/)
