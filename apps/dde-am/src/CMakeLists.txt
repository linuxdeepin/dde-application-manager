add_executable(${DDE_AM_BIN} main.cpp launcher.cpp commandexecutor.cpp)

target_link_libraries(${DDE_AM_BIN} PRIVATE dde_am_static Dtk6::Core)

target_include_directories(${DDE_AM_BIN} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS ${DDE_AM_BIN} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
