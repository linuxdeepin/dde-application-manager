set(LAUNCHAPP_BIN launchApp)

add_executable(${LAUNCHAPP_BIN} main.cpp)

target_link_libraries(${LAUNCHAPP_BIN} PRIVATE
    Qt6::Core
    Qt6::DBus
)
