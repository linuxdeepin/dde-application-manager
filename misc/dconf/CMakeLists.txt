set(DCONFIG_FILES
    com.deepin.dde.launcher.json
    com.deepin.dde.dock.json
    com.deepin.dde.appearance.json
)

install(FILES ${DCONFIG_FILES} DESTINATION /usr/share/dsg/configs/dde-application-manager)
