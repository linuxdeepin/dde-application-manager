set(SYSTEMD_FILES
    org.desktopspec.application.instance@.service
    org.desktopspec.application.manager.service
)

install(FILES ${SYSTEMD_FILES} DESTINATION lib/systemd/user/)
