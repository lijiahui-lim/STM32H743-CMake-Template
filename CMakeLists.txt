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
set(CMAKE_PROJECT_NAME fire_stm32)

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

# Add LVGL
find_package(Git QUIET)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/lvgl/CMakeLists.txt)
    message(STATUS "LVGL not found, cloning...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone https://gitee.com/mirrors/lvgl.git ${CMAKE_SOURCE_DIR}/lvgl
        RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone LVGL")
    endif()
endif()
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS ON CACHE BOOL "" FORCE)
add_subdirectory(lvgl)

# LVGL needs CMSIS RTOS2 headers for FreeRTOS integration
target_include_directories(lvgl PRIVATE
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/RTOS2/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/Core/Inc
)
target_compile_definitions(lvgl PRIVATE
    USE_HAL_DRIVER
    STM32H743xx
)

# LVGL demos also need CMSIS/HAL headers
target_include_directories(lvgl_demos PRIVATE
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/RTOS2/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/Core/Inc
)
target_compile_definitions(lvgl_demos PRIVATE
    USE_HAL_DRIVER
    STM32H743xx
)

# Add YAFFS2: cloned on demand and kept out of this repository (like lvgl).
# The target definition lives here because the upstream yaffs2 tree does not
# ship a CMakeLists.txt.
set(YAFFS2_DIR ${CMAKE_SOURCE_DIR}/yaffs2)
if(NOT EXISTS ${YAFFS2_DIR}/direct/yaffsfs.c)
    message(STATUS "YAFFS2 not found, cloning...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone https://gh-proxy.org/github.com/Aleph-One-Ltd/yaffs2.git ${YAFFS2_DIR}
        RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone YAFFS2")
    endif()
endif()

add_library(YAFFS2 OBJECT)
target_sources(YAFFS2 PRIVATE
    ${YAFFS2_DIR}/core/yaffs_allocator.c
    ${YAFFS2_DIR}/core/yaffs_bitmap.c
    ${YAFFS2_DIR}/core/yaffs_cache.c
    ${YAFFS2_DIR}/core/yaffs_checkptrw.c
    ${YAFFS2_DIR}/core/yaffs_ecc.c
    ${YAFFS2_DIR}/core/yaffs_endian.c
    ${YAFFS2_DIR}/core/yaffs_guts.c
    ${YAFFS2_DIR}/core/yaffs_nameval.c
    ${YAFFS2_DIR}/core/yaffs_nand.c
    ${YAFFS2_DIR}/core/yaffs_packedtags1.c
    ${YAFFS2_DIR}/core/yaffs_packedtags2.c
    ${YAFFS2_DIR}/core/yaffs_summary.c
    ${YAFFS2_DIR}/core/yaffs_tagscompat.c
    ${YAFFS2_DIR}/core/yaffs_tagsmarshall.c
    ${YAFFS2_DIR}/core/yaffs_verify.c
    ${YAFFS2_DIR}/core/yaffs_yaffs1.c
    ${YAFFS2_DIR}/core/yaffs_yaffs2.c
    ${YAFFS2_DIR}/direct/yaffs_attribs.c
    ${YAFFS2_DIR}/direct/yaffs_error.c
    ${YAFFS2_DIR}/direct/yaffsfs.c
    ${YAFFS2_DIR}/direct/yaffs_hweight.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/YAFFS2/Src/yaffs_stm32_nandif.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/YAFFS2/Src/yaffs_osglue.c
)

target_include_directories(YAFFS2 PUBLIC
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/YAFFS2/Inc
    ${YAFFS2_DIR}/core
    ${YAFFS2_DIR}/direct
)

target_link_libraries(YAFFS2 PUBLIC stm32cubemx)

target_compile_options(YAFFS2 PRIVATE
    -Wno-unused-parameter
    -Wno-sign-compare
    -Wno-missing-field-initializers
    -include yaffs_cfg.h
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/cmsis_os2_compat.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lvgl
    lvgl_demos
    YAFFS2
)
