# Keep package filtering and upgrade cleanup on the same legacy-file allowlist.
set(SUNSHINE_INSTALLER_ASSET_EXCLUDES "gui\\*")
set(SUNSHINE_INSTALLER_GUI_EXCLUDES "")
set(SUNSHINE_INSTALLER_LEGACY_GUI_CLEANUP "")
if(BUILD_QT_UI)
    string(APPEND SUNSHINE_INSTALLER_ASSET_EXCLUDES ",web\\*")
    set(_sunshine_legacy_gui_files WebView2Loader.dll alkaidlab-plugin-stylus.dll)
    list(JOIN _sunshine_legacy_gui_files "," SUNSHINE_INSTALLER_GUI_EXCLUDES)
    foreach(_legacy_gui_file IN LISTS _sunshine_legacy_gui_files)
        string(APPEND SUNSHINE_INSTALLER_LEGACY_GUI_CLEANUP
            "Type: files; Name: \"{app}\\assets\\gui\\${_legacy_gui_file}\"\n")
    endforeach()
endif()
