/**
 * =============================================================================
 * build.gradle.kts - App 模块构建配置
 * =============================================================================
 * 
 * 本文件是 Android App 模块的构建脚本，使用 Kotlin DSL 语法。
 * 
 * 【项目结构与构建的关系】
 * 
 * 项目根目录/
 * ├── build.gradle.kts              # 根级别构建配置
 * ├── settings.gradle.kts           # 项目模块配置
 * ├── gradle.properties             # Gradle 属性（缓存路径等）
 * │
 * └── app/
 *     └── build.gradle.kts          # 本文件 - App 模块构建配置
 * 
 * 【构建流程概要】
 * 
 * 1. Gradle 读取 settings.gradle.kts，确定要构建的模块
 * 2. 执行根目录的 build.gradle.kts（应用插件）
 * 3. 执行 app/build.gradle.kts（编译 Java/Kotlin，调用 CMake）
 * 4. CMake 编译 C++ 代码，生成 .so 共享库
 * 5. 打包所有资源（.so、.dex、assets 等）成 APK
 * 
 * =============================================================================
 */

// =============================================================================
// 插件声明
// =============================================================================

/**
 * 使用 plugins DSL 声明要应用的 Gradle 插件。
 * 
 * alias(libs.plugins.android.application) - Android 应用插件
 * alias(libs.plugins.kotlin.android) - Kotlin Android 插件
 * 
 * 插件版本在 gradle/libs.versions.toml 中统一管理。
 */
plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

// =============================================================================
// Android 配置块
// =============================================================================

/**
 * android {} 块包含所有 Android 特定的构建配置。
 */
android {
    // -------------------------------------------------------------------------
    // 命名空间
    // -------------------------------------------------------------------------
    /**
     * namespace: 声明应用的包名（必须与 AndroidManifest.xml 中的 package 一致）
     * 
     * 用途：
     * - 生成 R.java 的包名
     * - 避免资源冲突
     */
    namespace = "com.odinaris.bgfx_android_demo"

    // -------------------------------------------------------------------------
    // 编译 SDK 版本
    // -------------------------------------------------------------------------
    /**
     * compileSdk: 编译时使用的 Android SDK 版本
     * 
     * 决定可以使用哪些 API（由系统 SDK 提供）。
     * 更高版本可以访问更多新 API，但需要对应的构建工具。
     * 
     * 注意：这不是应用支持的最低或目标 Android 版本！
     */
    compileSdk = 34

    // -------------------------------------------------------------------------
    // 默认配置
    // -------------------------------------------------------------------------
    defaultConfig {
        /**
         * applicationId: 应用的唯一标识符
         * 
         * 在 Google Play 和设备上唯一标识应用。
         * 格式：通常是反转的域名，如 com.example.app
         */
        applicationId = "com.odinaris.bgfx_android_demo"

        /**
         * minSdk: 应用支持的最低 Android 版本
         * 
         * Android 5.0 (API 21) = 90%+ 设备
         * Android 6.0 (API 23) = 85%+ 设备
         * Android 10 (API 29) = 80%+ 设备
         * Android 13 (API 33) = 60%+ 设备
         * Android 14 (API 34) = 40%+ 设备（当前设置）
         */
        minSdk = 34

        /**
         * targetSdk: 目标 Android 版本
         * 
         * 声明应用针对哪个 Android 版本进行优化。
         * 某些行为会按照目标版本的处理方式运行。
         * 应定期更新以跟随最新 Android 版本。
         */
        targetSdk = 34

        /**
         * versionCode: 版本号（整数）
         * 
         * 用于应用商店和设备判断版本高低。
         * 必须递增，用于版本比较和升级判断。
         */
        versionCode = 1

        /**
         * versionName: 版本名（字符串）
         * 
         * 展示给用户的版本号，如 "1.0.0"
         */
        versionName = "1.0"

        /**
         * testInstrumentationRunner: 测试运行器
         * 
         * 用于运行 Android 单元测试和集成测试。
         */
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // -------------------------------------------------------------------------
        // NDK 配置
        // -------------------------------------------------------------------------
        /**
         * ndk {}: 原生开发工具包配置
         * 
         * NDK 允许开发者在 Android 应用中使用 C/C++ 代码。
         * bgfx 是 C++ 库，所以需要 NDK。
         */
        ndk {
            /**
             * abiFilters: 应用支持的 CPU 架构
             * 
             * Android 设备使用不同的 CPU 架构：
             * - armeabi-v7a: ARM 32位（第5-6代，2010年后设备）
             * - arm64-v8a:   ARM 64位（第8代，2014年后设备）【推荐】
             * - x86:         Intel 32位（模拟器、部分平板）
             * - x86_64:      Intel 64位（模拟器）
             * 
             * 【构建输出】
             * 每个 ABI 都会生成对应的 .so 文件：
             * lib/arm64-v8a/libbgfx_android_demo.so
             * lib/armeabi-v7a/libbgfx_android_demo.so
             * 
             * 【注意】当前仅构建了 arm64-v8a 的 bgfx 预编译库，
             * 因此暂时只支持 arm64-v8a 架构。
             */
            abiFilters += listOf("arm64-v8a")
        }
    }

    // -------------------------------------------------------------------------
    // 构建类型
    // -------------------------------------------------------------------------
    buildTypes {
        /**
         * release {}: 发布版本配置
         */
        release {
            /**
             * isMinifyEnabled: 是否启用代码混淆/压缩
             * 
             * true = 启用 R8 压缩（移除未使用的代码、重命名混淆）
             * false = 禁用（调试时使用）
             */
            isMinifyEnabled = false

            /**
             * proguardFiles: ProGuard/R8 规则文件
             * 
             * ProGuard 可以：
             * - 移除未使用的类和方法（减小 APK 大小）
             * - 混淆命名（增加逆向难度）
             * 
             * getDefaultProguardFile() 获取 Android SDK 内置的默认规则。
             */
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    // -------------------------------------------------------------------------
    // Java/Kotlin 编译选项
    // -------------------------------------------------------------------------
    compileOptions {
        /**
         * sourceCompatibility: 源代码使用的 Java 版本
         * 
         * 决定编译器使用哪个版本的 Java 语法和 API。
         * VERSION_1_8 = Java 8（当前设置）
         */
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }

    kotlinOptions {
        /**
         * jvmTarget: Kotlin 编译器生成的字节码目标版本
         * 
         * 应与 sourceCompatibility/targetCompatibility 匹配。
         */
        jvmTarget = "1.8"
    }

    // -------------------------------------------------------------------------
    // CMake 配置（原生代码）
    // -------------------------------------------------------------------------
    externalNativeBuild {
        cmake {
            /**
             * path: CMakeLists.txt 的路径（相对于本文件）
             * 
             * 指定 CMake 构建脚本的位置。
             */
            path = file("src/main/cpp/CMakeLists.txt")

            /**
             * version: CMake 版本
             * 
             * 必须与 Android NDK 自带的 CMake 版本兼容。
             * Android NDK 26 自带 CMake 3.22.1
             */
            version = "3.22.1"
        }
    }

    // -------------------------------------------------------------------------
    // 构建特性
    // -------------------------------------------------------------------------
    buildFeatures {
        /**
         * viewBinding: 启用 View Binding
         * 
         * View Binding 会为每个 XML 布局文件生成绑定类，
         * 提供类型安全的 View 访问方式。
         * 
         * 相比 findViewById：
         * - 编译时检查，无需类型转换
         * - 更快（直接引用，无运行时查找）
         * - 更安全（编译时检查，消除 ClassCastException 风险）
         */
        viewBinding = true
    }
}

// =============================================================================
// 依赖声明
// =============================================================================

/**
 * dependencies {} 块声明应用依赖的库。
 * 
 * 【依赖来源】
 * - Google 的 AndroidX 库（androidx.*）
 * - Maven Central 的第三方库（找不到时）
 * - 本地模块
 */
dependencies {
    /**
     * implementation: 运行时依赖
     * 
     * implementation vs api vs compileOnly:
     * - implementation: 依赖仅对本模块可见（编译快，推荐）
     * - api: 依赖对本模块和依赖本模块的模块可见（相当于旧版 compile）
     * - compileOnly: 仅编译时需要，运行时不需要（如注解处理器）
     */

    // AndroidX Core Kotlin 扩展
    implementation(libs.androidx.core.ktx)

    // AndroidX AppCompat（提供向后兼容的 Activity 等）
    implementation(libs.androidx.appcompat)

    // Material Design 组件库
    implementation(libs.material)

    // ConstraintLayout 布局管理器
    implementation(libs.androidx.constraintlayout)

    // -------------------------------------------------------------------------
    // 测试依赖
    // -------------------------------------------------------------------------
    
    /** JUnit 4 单元测试框架 */
    testImplementation(libs.junit)

    /** AndroidX 测试扩展 */
    androidTestImplementation(libs.androidx.junit)

    /** Espresso UI 测试框架 */
    androidTestImplementation(libs.androidx.espresso.core)
}
