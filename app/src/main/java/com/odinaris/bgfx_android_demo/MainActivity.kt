/**
 * =============================================================================
 * MainActivity - Android 主界面 / 渲染入口
 * =============================================================================
 * 
 * 本 Activity 是 Android 应用的入口界面，负责：
 * 1. 显示 SurfaceView 用于渲染
 * 2. 管理 Surface 的生命周期
 * 3. 通过 JNI 调用 C++ 层的渲染代码
 * 
 * 【渲染架构总览】
 * 
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                        MainActivity.kt                            │
 * │                                                                  │
 * │   ┌──────────────────────────────────────────────────────────┐   │
 * │   │                    SurfaceView                          │   │
 * │   │                   (渲染画布)                              │   │
 * │   └──────────────────────────────────────────────────────────┘   │
 * │                              │                                   │
 * │                    ┌─────────┴─────────┐                         │
 * │                    │  JNI 桥接          │                         │
 * │                    │  initBgfx()       │                         │
 * │                    │  renderBgfxFrame() │                         │
 * │                    │  shutdownBgfx()    │                         │
 * │                    └─────────┬─────────┘                         │
 * └─────────────────────────────────────────────────────────────────┘
 *                                  │
 *                                  ▼
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                      jni_binder.cpp                            │
 * │                                                                  │
 * │   s_renderer = BgfxRenderer()                                   │
 * │   s_renderer->initialize(width, height, window)                  │
 * │   s_renderer->render()                                           │
 * │   delete s_renderer                                              │
 * └─────────────────────────────────────────────────────────────────┘
 *                                  │
 *                                  ▼
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                      bgfx_renderer.cpp                          │
 * │                                                                  │
 * │   【未来完整实现】                                                │
 * │   bgfx::init()                                                  │
 * │   bgfx::setViewRect()                                           │
 * │   bgfx::submit()                                                │
 * │   bgfx::frame()                                                 │
 * │   bgfx::shutdown()                                              │
 * └─────────────────────────────────────────────────────────────────┘
 * 
 * 【生命周期与渲染的关系】
 * 
 *   onCreate()          创建 Activity，设置布局
 *       │
 *       ▼
 *   surfaceCreated()    Surface 创建/找到 ──► 初始化 bgfx ──► 开始渲染循环
 *       │
 *       ▼
 *   surfaceChanged()   Surface 尺寸变化 ──► 可选：调整渲染目标
 *       │
 *       ▼
 *   surfaceDestroyed() Surface 销毁 ──► 关闭 bgfx
 *       │
 *       ▼
 *   onPause()          Activity 暂停 ──► 关闭 bgfx（确保资源释放）
 *       │
 *       ▼
 *   onResume()         Activity 恢复 ──► 重新初始化 bgfx（如需要）
 * 
 * =============================================================================
 */

package com.odinaris.bgfx_android_demo

import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.appcompat.app.AppCompatActivity
import com.odinaris.bgfx_android_demo.databinding.ActivityMainBinding

/**
 * @class MainActivity
 * @brief Android 主界面，负责渲染器生命周期管理
 * 
 * 实现了 SurfaceHolder.Callback 接口来监听 Surface 的创建、变化和销毁事件。
 * 使用 ViewBinding 来访问布局中的 View 元素。
 */
class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    // =========================================================================
    // 成员变量
    // =========================================================================

    /**
     * @brief ViewBinding 对象
     * 
     * ViewBinding 是一种类型安全的方式访问布局中的 View。
     * 自动生成的 ActivityMainBinding 类包含所有带 ID 的 View 引用。
     * 
     * 例如：binding.surfaceView 相当于 findViewById<SurfaceView>(R.id.surface_view)
     */
    private lateinit var binding: ActivityMainBinding

    /**
     * @brief bgfx 渲染器是否已初始化
     * 
     * 用于跟踪渲染器状态，避免在未初始化时调用渲染。
     * 也用于在 onPause/onResume 时判断是否需要重新初始化。
     */
    private var isBgfxInitialized = false

    // =========================================================================
    // Activity 生命周期方法
    // =========================================================================

    /**
     * @brief Activity 创建时调用
     * 
     * 设置布局文件，并注册 Surface 的回调监听器。
     * 
     * @param savedInstanceState 如果 Activity 被系统重建，提供之前保存的状态数据
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        // 调用父类实现（必须）
        super.onCreate(savedInstanceState)

        // 使用 ViewBinding 填充布局
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // 注册 Surface 生命周期回调
        // 这样当 Surface 创建、变化或销毁时，会收到通知
        binding.surfaceView.holder.addCallback(this)
    }

    // =========================================================================
    // SurfaceHolder.Callback 实现
    // =========================================================================
    // SurfaceHolder.Callback 是 Surface 生命周期监听接口
    // 实现这个接口的 Activity 会在 Surface 状态变化时收到回调

    /**
     * @brief Surface 创建时调用
     * 
     * 这是初始化 bgfx 的最佳时机，因为此时 Surface 已经可用。
     * 
     * 【重要】此方法可能在 surface 创建后立即调用，
     * 此时 SurfaceView 可能还没有正确的尺寸。
     * 如果尺寸为 0，bgfx 初始化可能会失败。
     * 
     * @param holder Surface 持有者（包含 Surface 信息）
     */
    override fun surfaceCreated(holder: SurfaceHolder) {
        // 获取 SurfaceView 的尺寸
        // 注意：此时可能返回 0，因为 SurfaceView 可能还未完成布局
        val width = binding.surfaceView.width
        val height = binding.surfaceView.height

        // 调用 JNI 方法初始化 bgfx
        // 传入宽度、高度和 Java Surface 对象
        isBgfxInitialized = initBgfx(width, height, holder.surface)

        // 如果初始化成功，启动渲染循环
        if (isBgfxInitialized) {
            startRenderLoop()
        }
    }

    /**
     * @brief Surface 尺寸或格式变化时调用
     * 
     * 当设备的屏幕方向改变、分辨率变化或 Surface 被重新创建时会触发。
     * 
     * 对于 bgfx，可能需要：
     * 1. 更新内部分辨率设置
     * 2. 重新创建渲染资源（如 viewport、framebuffers）
     * 
     * @param holder Surface 持有者
     * @param format 新的像素格式（PixelFormat，如 ARGB_8888）
     * @param width  新的宽度
     * @param height 新的高度
     */
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // 当前 Stub 实现不做任何处理
        // 完整实现可在此处调用 bgfx::reset(width, height) 或重新初始化
    }

    /**
     * @brief Surface 销毁时调用
     * 
     * 收到此回调时，Surface 即将被销毁，必须立即关闭 bgfx。
     * 否则可能会导致 ANativeWindow 句柄失效后仍被访问。
     * 
     * @param holder Surface 持有者
     */
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        // 关闭 bgfx 渲染器
        if (isBgfxInitialized) {
            shutdownBgfx()
            isBgfxInitialized = false
        }
    }

    // =========================================================================
    // 渲染循环
    // =========================================================================

    /**
     * @brief 启动渲染循环
     * 
     * 使用 SurfaceView.post() 实现递归渲染循环。
     * 每次渲染一帧后，调度下一帧的渲染。
     * 
     * 【工作原理】
     * 1. 调用 renderBgfxFrame() 渲染当前帧
     * 2. 使用 SurfaceView.post() 将下一个渲染任务添加到消息队列
     * 3. 当 SurfaceView 准备好渲染下一帧时，执行下一个任务
     * 4. 重复步骤 1-3
     * 
     * 【性能考虑】
     * - 这种方式与 SurfaceView 的刷新率同步（通常 60fps）
     * - 如果渲染速度跟不上，会自动跳帧
     * - 对于复杂的渲染场景，可以考虑使用 Choreographer
     * 
     * 【停止条件】
     * 当 isBgfxInitialized 变为 false 时，循环停止
     */
    private fun startRenderLoop() {
        if (isBgfxInitialized) {
            // 渲染一帧
            renderBgfxFrame()

            // 调度下一帧渲染
            // post {} 将 Runnable 发送到 UI 线程的消息队列
            binding.surfaceView.post {
                startRenderLoop()  // 递归调用，形成循环
            }
        }
        // 如果 isBgfxInitialized 为 false，循环自然停止
    }

    // =========================================================================
    // Activity 暂停/恢复
    // =========================================================================

    /**
     * @brief Activity 暂停时调用
     * 
     * 当 Activity 失去焦点（如打开另一个 Activity）时调用。
     * 必须关闭 bgfx 以释放 GPU 资源，避免影响其他应用。
     */
    override fun onPause() {
        super.onPause()

        // 确保 bgfx 已关闭
        if (isBgfxInitialized) {
            shutdownBgfx()
            isBgfxInitialized = false
        }
    }

    /**
     * @brief Activity 恢复时调用
     * 
     * 当 Activity 重新获得焦点时调用。
     * 如果之前关闭了 bgfx，需要重新初始化。
     */
    override fun onResume() {
        super.onResume()

        // 如果 bgfx 未初始化，尝试重新初始化
        if (!isBgfxInitialized) {
            val holder = binding.surfaceView.holder

            // 只有当 Surface 有效时才初始化
            if (holder.surface.isValid) {
                surfaceCreated(holder)
            }
        }
    }

    // =========================================================================
    // JNI 方法声明
    // =========================================================================
    // 使用 external 关键字声明原生方法。
    // Kotlin 编译器会负责查找并调用对应的 C++ 函数。
    // 函数名映射规则：Java_com_odinaris_bgfx_1android_1demo_MainActivity_<methodName>

    /**
     * @brief 初始化 bgfx 渲染器
     * 
     * @param width  Surface 宽度（像素）
     * @param height Surface 高度（像素）
     * @param surface Android Surface 对象
     * @return true  初始化成功
     * @return false 初始化失败
     * 
     * 对应 JNI 函数：Java_com_odinaris_bgfx_1android_1demo_MainActivity_initBgfx
     */
    external fun initBgfx(width: Int, height: Int, surface: android.view.Surface): Boolean

    /**
     * @brief 关闭 bgfx 渲染器
     * 
     * 释放所有渲染资源。
     * 调用后，渲染器状态变为未初始化。
     * 
     * 对应 JNI 函数：Java_com_odinaris_bgfx_1android_1demo_MainActivity_shutdownBgfx
     */
    external fun shutdownBgfx()

    /**
     * @brief 渲染一帧
     * 
     * 每调用一次，bgfx 执行一帧的渲染工作。
     * 通常在渲染循环中每帧调用一次。
     * 
     * 对应 JNI 函数：Java_com_odinaris_bgfx_1android_1demo_MainActivity_renderBgfxFrame
     */
    external fun renderBgfxFrame()

    /**
     * @brief 查询渲染器是否已初始化
     * @return true 已初始化
     * @return false 未初始化或已关闭
     * 
     * 对应 JNI 函数：Java_com_odinaris_bgfx_1android_1demo_MainActivity_isBgfxInitialized
     */
    external fun isBgfxInitialized(): Boolean

    // =========================================================================
    // 静态初始化块
    // =========================================================================

    /**
     * @brief 静态初始化块
     * 
     * 在类加载时执行（首次访问 MainActivity 时）。
     * 用于加载原生共享库（.so 文件）。
     * 
     * 【库名映射】
     * System.loadLibrary("bgfx_android_demo")
     * 
     * Android 会自动查找：
     * - libbgfx_android_demo.so（在 APK 的 lib/ 目录下）
     * 
     * CMakeLists.txt 中 add_library(${CMAKE_PROJECT_NAME} SHARED ...)
     * 定义了这个库的名称。
     */
    companion object {
        init {
            System.loadLibrary("bgfx_android_demo")
        }
    }
}
