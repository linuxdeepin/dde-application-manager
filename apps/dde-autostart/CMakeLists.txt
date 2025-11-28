set(BIN_NAME dde-autostart)

set(SRCS
    main.cpp
)

add_executable(${BIN_NAME} ${SRCS})

target_link_libraries(${BIN_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Dtk6::Core
    dde_am_utils
)

install(TARGETS ${BIN_NAME}
    DESTINATION ${AM_LIBEXEC_DIR}
)
