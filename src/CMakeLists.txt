# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# Conditionally require bt component only if Improv WiFi is enabled
# This prevents BLE component from being linked (and its static initializers from running)
# when Improv WiFi is disabled, which may help avoid static init crashes
if(CONFIG_USE_IMPROV_WIFI)
    idf_component_register(
        SRCS ${app_sources}
        REQUIRES bt
    )
else()
    idf_component_register(
        SRCS ${app_sources}
    )
endif()
