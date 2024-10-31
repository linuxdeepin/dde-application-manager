add_executable(${DDE_AM_COMPATIBILITY_BIN} main.cpp
    launcher.h launcher.cpp
)

target_link_libraries(${DDE_AM_COMPATIBILITY_BIN} PRIVATE
    dde_am_static
    Dtk6::Core
)

target_include_directories(${DDE_AM_COMPATIBILITY_BIN} PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src
)

install(TARGETS ${DDE_AM_COMPATIBILITY_BIN} DESTINATION ${CMAKE_INSTALL_BINDIR})
