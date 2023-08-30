# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later
include(GNUInstallDirs)

# # systemd service
configure_file(
    systemd/user/org.deepin.dde.ApplicationManager1.service.in
    systemd/user/org.deepin.dde.ApplicationManager1.service
    @ONLY
)

set(SYSTEMD_USER_FILE
    ${CMAKE_CURRENT_BINARY_DIR}/systemd/user/org.deepin.dde.ApplicationManager1.service
)

install(FILES ${SYSTEMD_USER_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/user/)

# # dbus activate
configure_file(
    dbus/org.deepin.dde.ApplicationManager1.service.in
    dbus/org.deepin.dde.ApplicationManager1.service
    @ONLY
)

set(DBUS_APPLICATION_MANAGER_SERVICE_FILE
    ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.deepin.dde.ApplicationManager1.service
)

# install to session service directory for now
install(FILES ${DBUS_APPLICATION_MANAGER_SERVICE_FILE}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/service)
