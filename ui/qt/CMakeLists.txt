cmake_minimum_required(VERSION 3.25)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(SunshinePanel VERSION 1.0.0 LANGUAGES CXX)
    enable_testing()
endif()

find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Network Widgets Quick QuickControls2 Svg)
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

find_package(Git QUIET)
set(PANEL_COMMIT "")
set(PANEL_COMMIT_TIME "")
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE PANEL_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "${GIT_EXECUTABLE}" log -1 --format=%cI
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE PANEL_COMMIT_TIME OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --symbolic-full-name HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE _panel_ref OUTPUT_STRIP_TRAILING_WHITESPACE)
    foreach(_ref IN ITEMS HEAD "${_panel_ref}")
        execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --git-path "${_ref}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE _ref_path OUTPUT_STRIP_TRAILING_WHITESPACE)
        get_filename_component(_ref_path "${_ref_path}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        if(EXISTS "${_ref_path}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_ref_path}")
        endif()
    endforeach()
endif()
configure_file(src/build_info.h.in generated/build_info.h @ONLY)
qt_add_library(sunshine_panel_updates STATIC src/update_controller.cpp src/update_controller.h)
target_link_libraries(sunshine_panel_updates PUBLIC Qt6::Core Qt6::Network Qt6::Gui)
target_include_directories(sunshine_panel_updates PUBLIC src PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_features(sunshine_panel_updates PUBLIC cxx_std_20)
if(WIN32)
    target_link_libraries(sunshine_panel_updates PRIVATE shell32)
endif()

qt_add_executable(sunshine-gui WIN32 MACOSX_BUNDLE src/main.cpp)
target_link_libraries(sunshine-gui PRIVATE sunshine_panel_client sunshine_panel_updates Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Svg)
get_filename_component(_panel_brand "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/brand" ABSOLUTE)
qt_add_resources(sunshine-gui panel_brand PREFIX "/brand" BASE "${_panel_brand}"
    FILES "${_panel_brand}/sunshine.svg")
if(WIN32)
    enable_language(RC)
    set(PANEL_ICON_FILE "${_panel_brand}/sunshine.ico")
    configure_file(cmake/panel_icon.rc.in generated/panel_icon.rc @ONLY)
    target_sources(sunshine-gui PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/panel_icon.rc")
    set(_panel_font "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/NotoSansCJKsc-Regular.otf")
    set_property(TARGET sunshine-gui APPEND PROPERTY LINK_DEPENDS "${_panel_font}")
    add_custom_command(TARGET sunshine-gui POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:sunshine-gui>/fonts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_panel_font}" "$<TARGET_FILE_DIR:sunshine-gui>/fonts/"
        VERBATIM)
elseif(APPLE)
    target_sources(sunshine-gui PRIVATE "${_panel_brand}/sunshine.icns")
    set_source_files_properties("${_panel_brand}/sunshine.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(sunshine-gui PROPERTIES MACOSX_BUNDLE_ICON_FILE sunshine.icns)
endif()
set_target_properties(sunshine-gui PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Sunshine"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.sunshine-stream.panel"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0")
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(sunshine-gui
    URI Sunshine.Panel VERSION 1.0
    QML_FILES
        qml/Main.qml qml/Theme.qml qml/PanelText.qml qml/PanelButton.qml qml/SettingCard.qml
        qml/WindowChrome.qml qml/PanelDialog.qml qml/DialogActions.qml
        qml/SettingGroup.qml qml/SegmentedControl.qml qml/AppTile.qml qml/SessionsPage.qml
        qml/SectionTitle.qml qml/Field.qml qml/Choice.qml qml/Toggle.qml qml/LineIcon.qml
        qml/HomePage.qml qml/AppsPage.qml qml/ClientsPage.qml
        qml/SettingsPage.qml qml/LogsPage.qml qml/ConnectionPage.qml qml/AboutPage.qml
    RESOURCES assets/sunshine-mark.svg assets/sunshine-mark-light.svg assets/sunshine-tray.svg)

if(WIN32)
    set(_panel_destination "assets/gui")
    install(FILES "${_panel_font}" DESTINATION "${_panel_destination}/fonts" COMPONENT gui)
    install(FILES assets/fonts/OFL-NotoSansSC.txt DESTINATION "${_panel_destination}/licenses" COMPONENT gui)
else()
    set(_panel_destination "bin")
endif()
install(TARGETS sunshine-gui
    BUNDLE DESTINATION . COMPONENT gui
    RUNTIME DESTINATION "${_panel_destination}" COMPONENT gui)
if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${_panel_destination}")
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT gui)
    include(InstallRequiredSystemLibraries)
    if(NOT CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        message(FATAL_ERROR "The Qt panel package requires the MSVC redistributable DLLs.")
    endif()
endif()
if(WIN32)
    set(_panel_mingw_deploy "")
    if(MINGW)
        set(_panel_mingw_deploy "
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM windows+pe)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND \"${CMAKE_OBJDUMP}\")
set(panel_mingw_bin \"$<TARGET_FILE_DIR:Qt6::Core>\")
include(\"${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployMinGW.cmake\")
")
    endif()
    qt_generate_deploy_script(TARGET sunshine-gui OUTPUT_SCRIPT _panel_deploy CONTENT "
set(QT_DEPLOY_BIN_DIR assets/gui)
set(QT_DEPLOY_LIB_DIR assets/gui)
set(QT_DEPLOY_PLUGINS_DIR assets/gui/plugins)
set(QT_DEPLOY_QML_DIR assets/gui/qml)
qt_deploy_qml_imports(TARGET sunshine-gui PLUGINS_FOUND plugins_found)
qt_deploy_runtime_dependencies(EXECUTABLE \"$<TARGET_FILE:sunshine-gui>\"
    ADDITIONAL_MODULES \${plugins_found} GENERATE_QT_CONF)
${_panel_mingw_deploy}
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
    qt_add_executable(panel_update_tests tests/update_test.cpp)
    target_link_libraries(panel_update_tests PRIVATE sunshine_panel_updates Qt6::Test)
    add_test(NAME panel_update_contract COMMAND panel_update_tests)
    set_tests_properties(panel_update_contract PROPERTIES TIMEOUT 45)
endif()
