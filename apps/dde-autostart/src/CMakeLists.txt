set(DDE_AUTOSTART_BIN dde-autostart)

add_executable(${DDE_AUTOSTART_BIN} main.cpp)

target_link_libraries(${DDE_AUTOSTART_BIN} PRIVATE Qt6::Core Qt6::DBus
                                                   dde_am_static)

install(TARGETS ${DDE_AUTOSTART_BIN} RUNTIME DESTINATION ${AM_LIBEXEC_DIR})
