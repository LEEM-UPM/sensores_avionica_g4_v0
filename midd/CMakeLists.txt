cmake_minimum_required(VERSION 3.22)

project(middleware)
add_library(middleware INTERFACE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

target_compile_definitions(middleware INTERFACE 
)

target_include_directories(middleware INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    system
    sensors
)

target_sources(middleware INTERFACE
    # sensors/pads.c
    # system/interrupts.c
    # system/mcu.c
)

target_link_directories(middleware INTERFACE
)

target_link_libraries(middleware INTERFACE
)

# Validate that STM32CubeMX code is compatible with C standard
if(CMAKE_C_STANDARD LESS 11)
    message(ERROR "Generated code requires C11 or higher")
endif()
