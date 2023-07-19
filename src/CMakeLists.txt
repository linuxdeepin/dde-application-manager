include(GNUInstallDirs)

set(BIN_NAME dde-application-manager)
set(LIB_NAME ddeam)

file(GLOB_RECURSE SRCS ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

list(REMOVE_ITEM SRCS "${PROJECT_SOURCE_DIR}/src/utils.cpp")

add_library(${LIB_NAME} ${SRCS})

target_include_directories(${LIB_NAME} PRIVATE
    ${PROJECT_BINARY_DIR}
)

target_link_libraries(${LIB_NAME} PUBLIC
    Threads::Threads
    ${AdaptorLib}
)

add_executable(${BIN_NAME} main.cpp utils.cpp)
target_link_libraries(${BIN_NAME} PRIVATE
    ${LIB_NAME}
)

install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
