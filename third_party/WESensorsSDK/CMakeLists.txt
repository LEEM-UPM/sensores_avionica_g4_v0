cmake_minimum_required(VERSION 3.22)

project(wsensdk)
add_library(wsensdk INTERFACE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

target_compile_definitions(wsensdk INTERFACE 
)

target_include_directories(wsensdk INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    platform
    WSEN_PADS_2511020213301
)

target_sources(wsensdk INTERFACE
    platform/platform.c
    WSEN_PADS_2511020213301/WSEN_PADS_2511020213301.c
)

target_link_directories(wsensdk INTERFACE
)

target_link_libraries(wsensdk INTERFACE
)

# Validate that STM32CubeMX code is compatible with C standard
if(CMAKE_C_STANDARD LESS 11)
    message(ERROR "Generated code requires C11 or higher")
endif()
