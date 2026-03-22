cmake_minimum_required(VERSION 3.22.1)

project("bgfx_android_demo")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Get the absolute path to project directories
# CMakeLists.txt is located at app/src/main/cpp/
get_filename_component(CPP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

# Get the absolute path to project root
get_filename_component(PROJECT_SRC ${CPP_SOURCE_DIR}/.. ABSOLUTE)    # main
get_filename_component(PROJECT_APP ${PROJECT_SRC}/.. ABSOLUTE)      # app
get_filename_component(PROJECT_ROOT ${PROJECT_APP}/.. ABSOLUTE)    # root
get_filename_component(ACTUAL_ROOT ${PROJECT_ROOT}/.. ABSOLUTE)     # should be project root

set(BGFX_DIR ${ACTUAL_ROOT}/3rdparty/bgfx)
set(BX_DIR ${CPP_SOURCE_DIR}/../3rdparty/bx)

message(STATUS "Project directories:")
message(STATUS "  CPP_SOURCE_DIR: ${CPP_SOURCE_DIR}")
message(STATUS "  ACTUAL_ROOT: ${ACTUAL_ROOT}")
message(STATUS "  BGFX_DIR: ${BGFX_DIR}")
message(STATUS "  BX_DIR: ${BX_DIR}")

# Note: bx and bgfx are included as git submodules
# For full bgfx integration, you need to:
# 1. Build bgfx using GENie: cd 3rdparty/bgfx && make android-arm64-release
# 2. Link against the built libraries in CMakeLists.txt

# Create the native library
add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    bgfx_renderer.cpp
    jni_binder.cpp
)

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CPP_SOURCE_DIR}
)

# Platform-specific defines for Android
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    PLATFORM_ANDROID=1
)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    android
    log
)
