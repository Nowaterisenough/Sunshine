# Qt is the default distribution. Keep stale Web UI cache values out of it.
option(BUILD_QT_UI "Build and package the Qt 6.12 native control panel" ON)
option(SUNSHINE_QT_ONLY "Build the native control panel without the streaming host" OFF)

if(SUNSHINE_QT_ONLY AND NOT BUILD_QT_UI)
    message(FATAL_ERROR "SUNSHINE_QT_ONLY requires BUILD_QT_UI=ON")
endif()

if(BUILD_QT_UI)
    if(BUILD_WEB_UI)
        message(STATUS "Qt distribution selected: disabling the cached legacy Web UI build")
    endif()
    set(BUILD_WEB_UI OFF CACHE BOOL "Build the legacy web UI via npm" FORCE)
    message(STATUS "Sunshine distribution UI: Qt 6.12 (legacy Tauri and Web UI excluded)")
else()
    option(BUILD_WEB_UI "Build the legacy web UI via npm" ON)
    message(STATUS "Sunshine distribution UI: legacy (explicit BUILD_QT_UI=OFF)")
endif()
