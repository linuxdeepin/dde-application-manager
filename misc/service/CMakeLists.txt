set(SERVICE_FILES
    dde-application-manager.service
)

macro(install_symlink filepath wantsdir)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/link/${wantsdir}/)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_INSTALL_PREFIX}/lib/systemd/user/${filepath} ${PROJECT_BINARY_DIR}/link/${wantsdir}/${filepath})
    install(FILES ${PROJECT_BINARY_DIR}/link/${wantsdir}/${filepath} DESTINATION lib/systemd/user/${wantsdir}/)
endmacro(install_symlink)

# 安装service文件到用户service目录
install(FILES ${SERVICE_FILES} DESTINATION lib/systemd/user/)

install_symlink(dde-application-manager.service dde-session-daemon.target.wants)

