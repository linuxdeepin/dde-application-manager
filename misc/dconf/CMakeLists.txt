set(DCONFIG_FILES
    com.deepin.dde.launcher.json
    com.deepin.dde.dock.json
    com.deepin.dde.appearance.json
    com.deepin.dde.startdde.json
    com.deepin.xsettings.json
)

install(FILES ${DCONFIG_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/dsg/configs/dde-application-manager)
