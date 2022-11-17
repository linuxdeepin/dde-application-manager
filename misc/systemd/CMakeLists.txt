set(SYSTEMD_FILES
    org.deepin.dde.Application1.Instance@.service
    org.deepin.dde.Application1.Manager.service
)

install(FILES ${SYSTEMD_FILES} DESTINATION lib/systemd/user/)
