/**
 * =============================================================================
 * JNI 绑定层 - Java/Kotlin 与 C++ 的桥接
 * =============================================================================
 * 
 * 本文件实现了 Java/Kotlin 代码与 C++ 原生代码之间的接口。
 * 
 * 【什么是 JNI？】
 * JNI（Java Native Interface）是 Java 平台提供的一种机制，
 * 允许 Java 代码调用由其他语言（如 C、C++）编写的原生函数。
 * 
 * 反过来，原生代码也可以通过 JNI 访问 Java 对象、调用 Java 方法。
 * 
 * 【函数命名规则】
 * JNI 函数的命名必须遵循特定规则：
 * Java_<package>_<ClassName>_<methodName>
 * 
 * 例如：
 * Java_com_odinaris_bgfx_1android_1demo_MainActivity_initBgfx
 * 
 * 解释：
 * - Java_                   : JNI 函数前缀
 * - com_odinaris_bgfx_1android_1demo  : 包名（. 替换为 _，下划线转义为 _1）
 * - MainActivity_            : 类名
 * - initBgfx                : 方法名
 * 
 * 【回调流程图】
 * 
 *   MainActivity.kt                         jni_binder.cpp              bgfx_renderer.cpp
 *        │                                        │                           │
 *        │──── initBgfx() ──────────────────────►│                           │
 *        │                                        │──── new BgfxRenderer() ──►│
 *        │                                        │                           │
 *        │                                        │──── initialize() ─────────►│
 *        │                                        │                           │
 *        │◄──────────── true/false ───────────────│◄──────────────────────────│
 *        │                                        │                           │
 *        │──── renderBgfxFrame() ───────────────►│                           │
 *        │                                        │──── render() ───────────►│
 *        │                                        │                           │
 *        │◄───────────────────────────────────────│◄──────────────────────────│
 *        │                                        │                           │
 *        │──── shutdownBgfx() ───────────────────►│                           │
 *        │                                        │──── delete renderer ──────►│
 *        │                                        │                           │
 * 
 * =============================================================================
 */

// =============================================================================
// 系统头文件
// =============================================================================

// JNI 标准头文件，定义 JNI 类型（jobject, jint, jboolean 等）和函数
#include <jni.h>

// Android 原生窗口类型定义
// ANativeWindow 是 Android 系统提供的原生窗口结构体
#include <android/native_window.h>

// JNI 与 Android 原生窗口的桥接函数
// 提供 ANativeWindow_fromSurface() 等函数
#include <android/native_window_jni.h>

// =============================================================================
// 项目头文件
// =============================================================================

// BgfxRenderer 类的声明
#include "bgfx_renderer.h"

// =============================================================================
// 全局变量
// =============================================================================

/**
 * @brief 渲染器单例指针
 * 
 * 使用静态变量存储渲染器实例，因为 JNI 回调不持有对象状态。
 * 
 * 【为什么用 static？】
 * JNI 回调是 C 函数，不能直接使用 C++ 对象的 this 指针。
 * 我们使用一个全局/静态指针来维护渲染器实例。
 * 
 * @note 这种方式在单 Activity 场景下工作正常。
 *       多 Activity 或多渲染器场景需要更复杂的管理机制。
 */
static BgfxRenderer* s_renderer = nullptr;

// =============================================================================
// JNI 函数实现
// =============================================================================

/**
 * @brief 使用 extern "C" 确保函数名不被 C++ 修饰
 * 
 * C++ 编译器会对函数名进行"名字修饰"（name mangling），
 * 以支持函数重载等功能。
 * 
 * JNI 期望的函数名是未修饰的，因此必须用 extern "C" 声明。
 */
extern "C" {

// =============================================================================
// initBgfx - 初始化 bgfx 渲染器
// =============================================================================

/**
 * @brief 初始化 bgfx 渲染器（JNI 绑定）
 * 
 * 对应 Kotlin 代码：
 * external fun initBgfx(width: Int, height: Int, surface: Surface): Boolean
 * 
 * 【函数参数详解】
 * @param env    JNI 环境指针，提供了访问 Java 对象、调用 Java 方法的接口
 * @param thiz   调用此方法的 Java 对象（MainActivity 实例）
 * @param width  Surface 的像素宽度
 * @param height Surface 的像素高度
 * @param surface Java Surface 对象（android.view.Surface）
 * @return       JNI 布尔值：JNI_TRUE（成功）或 JNI_FALSE（失败）
 * 
 * 【执行流程】
 * 1. 如果已有渲染器实例，先销毁（允许重新初始化）
 * 2. 从 Java Surface 创建 ANativeWindow
 * 3. 创建 BgfxRenderer 实例
 * 4. 调用 BgfxRenderer::initialize()
 * 5. 如果初始化失败，清理实例
 * 6. 返回成功/失败状态
 */
JNIEXPORT jboolean JNICALL
Java_com_odinaris_bgfx_1android_1demo_MainActivity_initBgfx(
    JNIEnv* env,                    // JNI 环境指针
    jobject thiz,                   // 调用者对象（MainActivity）
    jint width,                     // Surface 宽度
    jint height,                    // Surface 高度
    jobject surface)                // Java Surface 对象
{
    // 步骤 1：清理已有渲染器（如果存在）
    if (s_renderer != nullptr) {
        delete s_renderer;
        s_renderer = nullptr;
    }

    // 步骤 2：创建渲染器实例
    s_renderer = new BgfxRenderer();

    // 步骤 3：将 Java Surface 转换为 ANativeWindow
    // ANativeWindow 是 Android NDK 提供的原生窗口句柄
    // bgfx 需要这个句柄来创建 OpenGL ES 渲染上下文
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);

    // 步骤 4：调用渲染器的初始化方法
    bool success = s_renderer->initialize(width, height, window);

    // 步骤 5：如果初始化失败，清理资源
    if (!success) {
        delete s_renderer;
        s_renderer = nullptr;
        return JNI_FALSE;  // 返回失败
    }

    return JNI_TRUE;  // 返回成功
}

// =============================================================================
// shutdownBgfx - 关闭 bgfx 渲染器
// =============================================================================

/**
 * @brief 关闭 bgfx 渲染器（JNI 绑定）
 * 
 * 对应 Kotlin 代码：
 * external fun shutdownBgfx()
 * 
 * 【执行流程】
 * 1. 检查渲染器是否存在
 * 2. 销毁渲染器实例
 * 3. 将指针设为 nullptr
 * 
 * 注意：不需要显式关闭 ANativeWindow，
 *       因为 Surface 被销毁时它会自动失效。
 */
JNIEXPORT void JNICALL
Java_com_odinaris_bgfx_1android_1demo_MainActivity_shutdownBgfx(
    JNIEnv* env,                    // JNI 环境指针（未使用）
    jobject thiz)                   // 调用者对象（未使用）
{
    // 检查并销毁渲染器
    if (s_renderer != nullptr) {
        delete s_renderer;          // 调用析构函数，释放资源
        s_renderer = nullptr;      // 避免悬挂指针
    }
    // 如果 s_renderer 已经是 nullptr，什么都不做
}

// =============================================================================
// renderBgfxFrame - 渲染一帧
// =============================================================================

/**
 * @brief 渲染一帧（JNI 绑定）
 * 
 * 对应 Kotlin 代码：
 * external fun renderBgfxFrame()
 * 
 * 【渲染循环说明】
 * 
 * 在 Android 上，渲染循环通常通过以下方式实现：
 * 
 * 方式 1：使用 SurfaceView.post() 循环
 *   surfaceView.post { 
 *       renderBgfxFrame()
 *       surfaceView.post { ... }  // 递归调用自己
 *   }
 * 
 * 方式 2：使用 Choreographer 回调（更精确的帧时序）
 *   Choreographer.getInstance().postFrameCallback {
 *       renderBgfxFrame()
 *   }
 * 
 * 【bgfx::frame() 的作用】
 * - 执行所有待处理的绘制命令
 * - 切换前后缓冲区（双缓冲）
 * - 返回前一帧的完成状态
 */
JNIEXPORT void JNICALL
Java_com_odinaris_bgfx_1android_1demo_MainActivity_renderBgfxFrame(
    JNIEnv* env,                    // JNI 环境指针（未使用）
    jobject thiz)                   // 调用者对象（未使用）
{
    // 检查渲染器是否存在
    if (s_renderer != nullptr) {
        // 调用渲染器的渲染方法
        s_renderer->render();
    }
    // 如果渲染器不存在，静默忽略（可能尚未初始化）
}

// =============================================================================
// isBgfxInitialized - 查询初始化状态
// =============================================================================

/**
 * @brief 查询渲染器是否已初始化（JNI 绑定）
 * 
 * 对应 Kotlin 代码：
 * external fun isBgfxInitialized(): Boolean
 * 
 * 【使用场景】
 * - 在 resume 时判断是否需要重新初始化
 * - 在某些 UI 操作前检查渲染器状态
 * 
 * @return JNI 布尔值：JNI_TRUE（已初始化）或 JNI_FALSE（未初始化）
 */
JNIEXPORT jboolean JNICALL
Java_com_odinaris_bgfx_1android_1demo_MainActivity_isBgfxInitialized(
    JNIEnv* env,                    // JNI 环境指针（未使用）
    jobject thiz)                   // 调用者对象（未使用）
{
    // 检查渲染器是否存在且已初始化
    return (s_renderer != nullptr && s_renderer->isInitialized()) 
        ? JNI_TRUE   // 条件为真
        : JNI_FALSE;  // 条件为假
}

// =============================================================================
// 结束 extern "C" 块
// =============================================================================

} // extern "C"
