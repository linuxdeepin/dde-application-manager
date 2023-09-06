# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later
include(GNUInstallDirs)

macro(install_symlink filepath wantsdir)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/link/${wantsdir}/)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_INSTALL_PREFIX}/lib/systemd/user/${filepath} ${PROJECT_BINARY_DIR}/link/${wantsdir}/${filepath})
    install(FILES ${PROJECT_BINARY_DIR}/link/${wantsdir}/${filepath} DESTINATION lib/systemd/user/${wantsdir}/)
endmacro(install_symlink)

# # systemd service
configure_file(
    systemd/user/org.desktopspec.ApplicationManager1.service.in
    systemd/user/org.desktopspec.ApplicationManager1.service
    @ONLY
)

set(SYSTEMD_USER_FILE
    ${CMAKE_CURRENT_BINARY_DIR}/systemd/user/org.desktopspec.ApplicationManager1.service
)

set(SERVICE_DEST_PATH ${CMAKE_INSTALL_PREFIX}/lib/systemd/user/)

install(FILES ${SYSTEMD_USER_FILE} DESTINATION ${SERVICE_DEST_PATH})

# create a symlink to dde-session
install_symlink(org.desktopspec.ApplicationManager1.service dde-session-initialized.target.wants)

# # dbus activate
configure_file(
    dbus/org.desktopspec.ApplicationManager1.service.in
    dbus/org.desktopspec.ApplicationManager1.service
    @ONLY
)

set(DBUS_APPLICATION_MANAGER_SERVICE_FILE
    ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.desktopspec.ApplicationManager1.service
)

# install to session service directory for now
install(FILES ${DBUS_APPLICATION_MANAGER_SERVICE_FILE}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)

# install dpkg hook
install(FILES ${CMAKE_CURRENT_LIST_DIR}/dpkg/dpkg.cfg.d/am-update-hook
    DESTINATION /etc/dpkg/dpkg.cfg.d)
