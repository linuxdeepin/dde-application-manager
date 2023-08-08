find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

add_executable(${APP_LAUNCH_HELPER_BIN} main.cpp)
target_link_libraries(${APP_LAUNCH_HELPER_BIN} PRIVATE
    PkgConfig::SYSTEMD
)

target_include_directories(${APP_LAUNCH_HELPER_BIN} PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

include(GNUInstallDirs)
install(TARGETS ${APP_LAUNCH_HELPER_BIN} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/deepin/application-manager/)
