set(SYSTEMD_FILES
    org.deskspec.application.instance@.service
    org.deskspec.application.manager.service
)

install(FILES ${SYSTEMD_FILES} DESTINATION lib/systemd/user/)
