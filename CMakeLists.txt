cmake_minimum_required(VERSION 3.22)

#
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
add_executable(${CMAKE_PROJECT_NAME}
    Core/Inc/vectorNav.h)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
add_subdirectory(third_party/WESensorsSDK)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/vectorNav.c
    # poc/wsen_pads/main.c
    poc/VectorNav_VN100/main.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    app
    Core/Inc
    Drivers/STM32G4xx_HAL_Driver/Inc
    Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32G4xx/Include
    Drivers/CMSIS/Include
    # Add user defined include paths
)

# Link stm32cubemx for HAL includes and compiler paths
target_link_libraries(${CMAKE_PROJECT_NAME} stm32cubemx)

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
