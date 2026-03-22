cmake_minimum_required(VERSION 3.22.1)

project("bgfx_android_demo")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Get the absolute path to 3rdparty directory
get_filename_component(BGFX_3RDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty ABSOLUTE)

# Set paths for bgfx and bx
set(BGFX_DIR ${BGFX_3RDPARTY_DIR}/bgfx)
set(BX_DIR ${BGFX_3RDPARTY_DIR}/bx)

# Check if bx exists, if not, use FetchContent to get it
if(NOT EXISTS ${BX_DIR}/CMakeLists.txt AND NOT EXISTS ${BX_DIR}/include/bx/bx.h)
    message(STATUS "bx not found, will fetch from GitHub")
    include(FetchContent)
    FetchContent_Declare(
        bx
        GIT_REPOSITORY https://github.com/bkaradzic/bx.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        SOURCE_DIR ${BX_DIR}
    )
    FetchContent_MakeAvailable(bx)
endif()

# Ensure BX_DIR is set correctly
if(NOT EXISTS ${BX_DIR}/include/bx/bx.h)
    message(FATAL_ERROR "bx headers not found in ${BX_DIR}/include/bx/bx.h")
endif()

# Create the native library
add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    bgfx_renderer.cpp
    jni_binder.cpp
)

# Include directories for bgfx and bx
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BGFX_DIR}/include
    ${BGFX_DIR}/include/bgfx
    ${BX_DIR}/include
)

# Platform-specific defines for Android + OpenGL ES
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    BGFX_CONFIG_RENDERER_OPENGLES=1
    BGFX_BUILD_SHADERC=0
    BGFX_CONFIG_MAX_FRAME_BLIT=1
    PLATFORM_ANDROID=1
)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    android
    log
)
