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
set(CMAKE_PROJECT_NAME SteeringWheel)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    # ----- BSP板级驱动 -----
    BSP/bsp_can.c
    BSP/bsp_lcd.c
    BSP/bsp_ec200.c
    BSP/bsp_ws2812b.c
    BSP/bsp_w25q128.c
    BSP/bsp_usb.c
    BSP/bsp_input.c
    # ----- App应用层 -----
    App/app_can.c
    App/app_ec200.c
    App/app_simhub.c
    App/app_ws2812b.c
    App/app_input.c
    App/app_iap.c
    # ----- 全局变量 -----
    Core/Src/Variable.c
)

# ----- LVGL源文件（必须放在target_sources块外面）-----
file(GLOB_RECURSE LVGL_SOURCES "Middlewares/LVGL/src/*.c")
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${LVGL_SOURCES})

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    # ----- BSP和App头文件路径 -----
    BSP
    App
    # ----- LVGL头文件路径 -----
    Middlewares/LVGL
    Middlewares/LVGL/src
    Middlewares/LVGL/include
    Middlewares/LVGL/include/lvgl
    Middlewares/LVGL/porting
    #Middlewares/LVGL/ui
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# ----- 生成HEX和BIN -----
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Generating HEX and BIN files..."
)