set(BIN_NAME dde-application-manager)

add_executable(${BIN_NAME} main.cpp utils.cpp)

target_link_libraries(${BIN_NAME} PRIVATE
    dde_am_static
    Qt6::Gui
)

target_include_directories(${BIN_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

include(GNUInstallDirs)
install(TARGETS ${BIN_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
