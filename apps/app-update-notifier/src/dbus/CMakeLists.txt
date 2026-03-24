file(GLOB_RECURSE app_update_notifier_static_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

qt_add_dbus_adaptor(app_update_notifier_static_SOURCE ${CMAKE_CURRENT_LIST_DIR}/../../api/dbus/org.desktopspec.ApplicationUpdateNotifier1.xml dbus/applicationupdatenotifier1service.h ApplicationUpdateNotifier1Service)

add_library(app_update_notifier_static STATIC ${app_update_notifier_static_SOURCE})

target_link_libraries(
    app_update_notifier_static PUBLIC
    Qt6::Core
    Qt6::DBus
)

target_include_directories(
    app_update_notifier_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
