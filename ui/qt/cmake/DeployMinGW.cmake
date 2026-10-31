# MSYS2 Qt links system-provided libraries that windeployqt does not collect.
# Scan the deployed plugins too, since they are loaded after process startup.
set(panel_bin "${QT_DEPLOY_PREFIX}/${QT_DEPLOY_BIN_DIR}")
file(GLOB_RECURSE panel_libraries LIST_DIRECTORIES false "${panel_bin}/*.dll")
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${panel_bin}/sunshine-gui.exe"
    LIBRARIES ${panel_libraries}
    DIRECTORIES "${panel_bin}" "${panel_mingw_bin}"
    PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
    POST_EXCLUDE_REGEXES "[/\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\]" "[/\\][Ss][Yy][Ss][Ww][Oo][Ww]64[/\\]"
    RESOLVED_DEPENDENCIES_VAR panel_dependencies
    UNRESOLVED_DEPENDENCIES_VAR panel_missing)
if(panel_missing)
    message(FATAL_ERROR "Unresolved Qt MinGW runtime dependencies: ${panel_missing}")
endif()
foreach(dependency IN LISTS panel_dependencies)
    cmake_path(IS_PREFIX panel_mingw_bin "${dependency}" NORMALIZE from_toolchain)
    if(from_toolchain)
        file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/${QT_DEPLOY_BIN_DIR}"
            TYPE SHARED_LIBRARY FILES "${dependency}")
    endif()
endforeach()
