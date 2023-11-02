include(GNUInstallDirs)

file(GLOB DBusAPI ${CMAKE_CURRENT_LIST_DIR}/dbus/*.xml)

install(FILES ${DBusAPI}
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dde-application-manager/)
