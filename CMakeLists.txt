cmake_minimum_required(VERSION 3.22)

#z
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME sensores_avionica_g4_v0)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
add_subdirectory(third_party/WESensorsSDK)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    poc/wsen_hids/main.c
    # poc/wsen_hids/main.c
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    app
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    WE_USE_FLOAT
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    wsensdk
    # Add user defined libraries
)
# =========================================================
# NUEVO TARGET: FDCAN POC
# =========================================================

# 1. Creamos el nuevo ejecutable apuntando a TU archivo nuevo
add_executable(fdcan_test poc/fdcan_test/main.c)

# 2. Le damos acceso a las librerías del STM32 (HAL, CMSIS, etc.)
# Esto es vital para que funcionen HAL_Init, SystemClock_Config, etc.
target_link_libraries(fdcan_test PRIVATE
    stm32cubemx
)

# 3. Le decimos dónde están los archivos .h básicos (main.h, etc)
target_include_directories(fdcan_test PRIVATE
    Core/Inc
)

# 4. (Opcional) Si usas funciones matemáticas float, definimos esto también
target_compile_definitions(fdcan_test PRIVATE
    WE_USE_FLOAT
)