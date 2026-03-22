# =============================================================================
# Android NDK + bgfx 模板项目 - CMake 构建配置
# =============================================================================
# 
# 本文件是 Android CMake 构建系统的入口点，用于编译原生 C++ 代码库。
# Android Studio 通过 externalNativeBuild 机制调用此文件。
#
# 目录结构（相对于项目根目录）:
#   app/
#   ├── build.gradle.kts          # Android 构建配置
#   └── src/main/
#       ├── java/.../MainActivity.kt  # Java/Kotlin 层
#       ├── res/layout/activity_main.xml  # 布局文件
#       └── cpp/
#           ├── CMakeLists.txt   # 本文件 - CMake 构建配置
#           ├── native-lib.cpp  # 入口点（空实现）
#           ├── bgfx_renderer.h  # 渲染器抽象类头文件
#           ├── bgfx_renderer.cpp# 渲染器实现
#           └── jni_binder.cpp   # JNI 绑定层
#
# 第三方库位置:
#   3rdparty/bgfx/  # bgfx 图形库
#   3rdparty/bx/    # bx 数学库
#   3rdparty/bimg/  # bimg 图像库
#   3rdparty/prebuilt/android-arm64/  # 预编译的 bgfx/bx/bimg 静态库
#
# =============================================================================

# CMake 最低版本要求（Android NDK 26+ 自带 CMake 3.22.1）
cmake_minimum_required(VERSION 3.22.1)

# 定义项目名称（必须与 build.gradle.kts 中 android.defaultConfig.ndk.moduleName 一致）
project("bgfx_android_demo")

# =============================================================================
# C++ 标准配置
# =============================================================================
# bgfx/bx 需要 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# 路径计算
# =============================================================================
# CMakeLists.txt 位于 app/src/main/cpp/
# CMAKE_CURRENT_SOURCE_DIR 指向该目录

# 获取 cpp 目录的绝对路径
get_filename_component(CPP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

# 逐级向上获取项目根目录的绝对路径
# cpp -> main -> app -> 项目根目录
get_filename_component(PROJECT_SRC ${CPP_SOURCE_DIR}/.. ABSOLUTE)    # main
get_filename_component(PROJECT_APP ${PROJECT_SRC}/.. ABSOLUTE)      # app
get_filename_component(PROJECT_ROOT ${PROJECT_APP}/.. ABSOLUTE)    # 项目根目录
get_filename_component(ACTUAL_ROOT ${PROJECT_ROOT}/.. ABSOLUTE)     # 实际根目录（处理 Windows 路径问题）

# =============================================================================
# 第三方库路径配置
# =============================================================================
# 项目根目录下的 3rdparty 目录
set(THIRDPARTY_DIR ${ACTUAL_ROOT}/3rdparty)

# bgfx 图形库
set(BGFX_DIR ${THIRDPARTY_DIR}/bgfx)

# bx 数学/工具库
set(BX_DIR ${THIRDPARTY_DIR}/bx)

# bimg 图像处理库
set(BIMG_DIR ${THIRDPARTY_DIR}/bimg)

# 预编译库目录（我们在 Windows 上使用 GENie 交叉编译的 Android ARM64 库）
set(PREBUILT_DIR ${THIRDPARTY_DIR}/prebuilt/android-arm64)

# =============================================================================
# 调试信息：打印关键路径（CMake 配置时会显示）
# =============================================================================
message(STATUS "========================================")
message(STATUS "Android NDK + bgfx 项目 CMake 配置")
message(STATUS "========================================")
message(STATUS "CPP_SOURCE_DIR: ${CPP_SOURCE_DIR}")
message(STATUS "ACTUAL_ROOT: ${ACTUAL_ROOT}")
message(STATUS "BGFX_DIR: ${BGFX_DIR}")
message(STATUS "BX_DIR: ${BX_DIR}")
message(STATUS "BIMG_DIR: ${BIMG_DIR}")
message(STATUS "PREBUILT_DIR: ${PREBUILT_DIR}")
message(STATUS "========================================")

# =============================================================================
# 创建原生库
# =============================================================================
# add_library 创建共享库（.so 文件）
# Android Gradle Plugin 会自动将 .so 文件打包到 APK 的 lib/ 目录
add_library(${CMAKE_PROJECT_NAME} SHARED
    # 入口点文件（当前为空，JNI 绑定在 jni_binder.cpp 中）
    native-lib.cpp
    # bgfx 渲染器实现（包含初始化、渲染、销毁逻辑）
    bgfx_renderer.cpp
    # JNI 绑定层（Java/Kotlin <-> C++ 桥接）
    jni_binder.cpp
)

# =============================================================================
# 头文件包含路径
# =============================================================================
# CMake 的 PRIVATE 关键字表示这些路径只对当前目标可见
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # 当前 cpp 目录（包含本目录的头文件）
    ${CPP_SOURCE_DIR}
    
    # bgfx 头文件
    ${BGFX_DIR}/include
    
    # bx 头文件
    ${BX_DIR}/include
    
    # bimg 头文件
    ${BIMG_DIR}/include
)

# =============================================================================
# 平台特定宏定义
# =============================================================================
# PLATFORM_ANDROID: 标识代码运行在 Android 平台
# 用于条件编译某些平台相关代码
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    PLATFORM_ANDROID=1
    
    # bx 库配置
    BX_CONFIG_DEBUG=0  # bx 调试配置（0 = release）
    
    # 定义 BGFX_CONFIG_RENDERER_OPENGLES 为 1，启用 OpenGL ES 后端
    # Android 默认使用 OpenGL ES
    BGFX_CONFIG_RENDERER_OPENGLES=1
    
    # 禁用不需要的渲染后端，减小库大小
    BGFX_CONFIG_RENDERER_VULKAN=0
    BGFX_CONFIG_RENDERER_D3D11=0
    BGFX_CONFIG_RENDERER_D3D12=0
    BGFX_CONFIG_RENDERER_METAL=0
    BGFX_CONFIG_RENDERER_GNM=0
    BGFX_CONFIG_RENDERER_NVN=0
    BGFX_CONFIG_RENDERER_WEBGPU=0
)

# =============================================================================
# 链接预编译的 bgfx/bx/bimg 静态库
# =============================================================================
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    # 预编译的 bgfx/bx/bimg 库（使用 GENie 在 Windows 上交叉编译）
    ${PREBUILT_DIR}/libbgfxRelease.a
    ${PREBUILT_DIR}/libbimgRelease.a
    ${PREBUILT_DIR}/libbimg_decodeRelease.a
    ${PREBUILT_DIR}/libbxRelease.a
    
    # 系统库
    android     # 原生窗口和输入支持
    log         # Android 日志系统
    
    # OpenGL ES（bgfx Android 后端需要）
    GLESv3
    
    # EGL（bgfx OpenGL ES 后端需要，用于创建渲染上下文）
    EGL
)

# =============================================================================
# 构建输出
# =============================================================================
# 构建完成后，共享库位于：
# app/build/intermediates/cxx/Debug/<ABI>/libbgfx_android_demo.so
#
# APK 中的位置：
# lib/<ABI>/libbgfx_android_demo.so
# 例如：lib/arm64-v8a/libbgfx_android_demo.so
