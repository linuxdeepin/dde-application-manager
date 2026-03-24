set(BIN_NAME app-identifier)

add_executable(${BIN_NAME} main.cpp)

target_link_libraries(${BIN_NAME} PRIVATE
    Qt6::Core
    Qt6::DBus
)

target_include_directories(${BIN_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src
)

install(TARGETS ${BIN_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
