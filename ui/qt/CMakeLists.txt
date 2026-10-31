cmake_minimum_required(VERSION 3.25)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(SunshinePanel VERSION 1.0.0 LANGUAGES CXX)
    enable_testing()
endif()

find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Network Widgets Quick QuickControls2)
if(NOT Qt6_VERSION VERSION_LESS 6.13)
    message(FATAL_ERROR "The native panel targets Qt 6.12.x. Select a Qt 6.12 kit.")
endif()
qt_standard_project_setup(REQUIRES 6.12)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
option(SUNSHINE_QT_TESTS "Build native panel contract tests" ON)

qt_add_library(sunshine_panel_client STATIC src/sunshine_client.cpp src/sunshine_client.h)
target_link_libraries(sunshine_panel_client PUBLIC Qt6::Core Qt6::Network)
target_compile_features(sunshine_panel_client PUBLIC cxx_std_20)
target_include_directories(sunshine_panel_client PUBLIC src)

qt_add_executable(sunshine-gui WIN32 MACOSX_BUNDLE src/main.cpp)
target_link_libraries(sunshine-gui PRIVATE sunshine_panel_client Qt6::Quick Qt6::QuickControls2 Qt6::Widgets)
set_target_properties(sunshine-gui PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Sunshine"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.sunshine-stream.panel"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0")
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(sunshine-gui
    URI Sunshine.Panel VERSION 1.0
    QML_FILES
        qml/Main.qml qml/Theme.qml qml/PanelButton.qml qml/SettingCard.qml
        qml/WindowChrome.qml qml/PanelDialog.qml qml/DialogActions.qml
        qml/SettingGroup.qml qml/SegmentedControl.qml qml/AppTile.qml qml/SessionsPage.qml
        qml/SectionTitle.qml qml/Field.qml qml/Choice.qml qml/Toggle.qml qml/LineIcon.qml
        qml/HomePage.qml qml/AppsPage.qml qml/ClientsPage.qml
        qml/SettingsPage.qml qml/LogsPage.qml qml/ConnectionPage.qml
    RESOURCES assets/sunshine.svg assets/sunshine-mark.svg)

if(WIN32)
    set(_panel_destination "assets/gui")
else()
    set(_panel_destination "bin")
endif()
install(TARGETS sunshine-gui
    BUNDLE DESTINATION . COMPONENT gui
    RUNTIME DESTINATION "${_panel_destination}" COMPONENT gui)
if(WIN32)
    qt_generate_deploy_script(TARGET sunshine-gui OUTPUT_SCRIPT _panel_deploy CONTENT "
set(QT_DEPLOY_BIN_DIR assets/gui)
set(QT_DEPLOY_LIB_DIR assets/gui)
set(QT_DEPLOY_PLUGINS_DIR assets/gui/plugins)
set(QT_DEPLOY_QML_DIR assets/gui/qml)
qt_deploy_qml_imports(TARGET sunshine-gui PLUGINS_FOUND plugins_found)
qt_deploy_runtime_dependencies(EXECUTABLE \"$<TARGET_FILE:sunshine-gui>\"
    ADDITIONAL_MODULES \${plugins_found} GENERATE_QT_CONF)
")
else()
    qt_generate_deploy_qml_app_script(TARGET sunshine-gui OUTPUT_SCRIPT _panel_deploy
        NO_UNSUPPORTED_PLATFORM_ERROR)
endif()
install(SCRIPT "${_panel_deploy}" COMPONENT gui)

if(SUNSHINE_QT_TESTS)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Test)
    enable_testing()
    qt_add_executable(panel_client_tests tests/client_test.cpp)
    target_link_libraries(panel_client_tests PRIVATE sunshine_panel_client Qt6::Test)
    add_test(NAME panel_client_contract COMMAND panel_client_tests)
    set_tests_properties(panel_client_contract PROPERTIES TIMEOUT 45)
endif()
