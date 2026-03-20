add_executable(${DDE_AM_BIN} main.cpp launcher.h launcher.cpp commandexecutor.h
                             commandexecutor.cpp)

target_link_libraries(${DDE_AM_BIN} PRIVATE dde_am_static Dtk6::Core)

target_include_directories(${DDE_AM_BIN} PRIVATE ${PROJECT_SOURCE_DIR}/src
                                                 ${PROJECT_BINARY_DIR}/src)

install(TARGETS ${DDE_AM_BIN} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
