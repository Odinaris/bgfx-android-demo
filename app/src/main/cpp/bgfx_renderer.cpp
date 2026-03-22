/**
 * =============================================================================
 * BgfxRenderer - bgfx 渲染器实现
 * =============================================================================
 * 
 * 本文件包含 BgfxRenderer 类的方法实现。
 * 
 * 【bgfx 渲染流程】
 * 1. 初始化：bgfx::init() -> 创建几何体 -> 创建着色器
 * 2. 渲染循环：设置变换 -> 提交绘制 -> bgfx::frame()
 * 3. 关闭：销毁资源 -> bgfx::shutdown()
 * 
 * 【三角形渲染示例】
 * 本实现创建一个简单的彩色三角形，使用内嵌着色器（无外部依赖）。
 * 
 * =============================================================================
 */

// =============================================================================
// 头文件
// =============================================================================

#include "bgfx_renderer.h"

// 嵌入式 shader 数据
#include "shaders/essl/shaders_essl.h"

// =============================================================================
// 顶点数据定义
// =============================================================================

/**
 * @brief 顶点格式：位置（3 floats）+ 颜色（4 bytes）
 */
struct PosColorVertex {
    float x, y, z;    // 位置
    uint32_t abgr;    // ARGB 颜色（小端序）
};

// 三角形顶点数据（逆时针顺序）
static PosColorVertex s_triangleVertices[] = {
    {  0.0f,  0.5f, 0.0f, 0xFF00FF00 }, // 顶部 - 绿色
    { -0.5f, -0.5f, 0.0f, 0xFF0000FF }, // 左下 - 蓝色
    {  0.5f, -0.5f, 0.0f, 0xFFFF0000 }, // 右下 - 红色
};

// 三角形索引数据
static const uint16_t s_triangleIndices[] = {
    0, 1, 2  // 一个三角形
};

// 顶点布局描述
static bgfx::VertexLayout s_vertexLayout;

// =============================================================================
// BgfxRenderer 构造函数
// =============================================================================

BgfxRenderer::BgfxRenderer()
    : m_initialized(false)
    , m_width(0)
    , m_height(0)
    , m_window(nullptr)
    , m_vbh(bgfx::kInvalidHandle)
    , m_ibh(bgfx::kInvalidHandle)
    , m_program(bgfx::kInvalidHandle)
    , m_time(0.0f)
{
    // 初始化顶点布局（如果尚未初始化）
    if (s_vertexLayout.getStride() == 0) {
        s_vertexLayout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
            .end();
    }
}

// =============================================================================
// BgfxRenderer 析构函数
// =============================================================================

BgfxRenderer::~BgfxRenderer()
{
    shutdown();
}

// =============================================================================
// initialize - 初始化渲染器
// =============================================================================

bool BgfxRenderer::initialize(int width, int height, void* window)
{
    // 防止重复初始化
    if (m_initialized) {
        LOGE("BgfxRenderer already initialized!");
        return false;
    }

    // 记录 Surface 尺寸和窗口句柄
    m_width = width;
    m_height = height;
    m_window = window;
    m_time = 0.0f;

    // ------------------- bgfx 初始化 -------------------
    
    // 1. 创建初始化参数结构体
    bgfx::Init init;
    
    // 2. 选择渲染后端：自动检测（Android 上通常会选 OpenGLES 或 Vulkan）
    // 设置为 Count 表示让 bgfx 自动选择合适的渲染器
    init.type = bgfx::RendererType::Count;
    
    // 3. 设置帧缓冲分辨率
    init.resolution.width = (uint32_t)width;
    init.resolution.height = (uint32_t)height;
    init.resolution.reset = BGFX_RESET_VSYNC;  // 启用垂直同步
    
    // 4. 设置平台相关数据
    // 这些字段告诉 bgfx 如何与 Android 系统交互
    init.platformData.ndt = nullptr;           // Native Display Type
    init.platformData.nwh = window;             // Native Window Handle (ANativeWindow*)
    init.platformData.context = nullptr;        // OpenGL Context（可选）
    init.platformData.backBuffer = nullptr;     // Back Buffer（可选）
    init.platformData.backBufferDS = nullptr;   // Back Buffer Depth Stencil（可选）
    
    // 5. 调用 bgfx 初始化
    if (!bgfx::init(init)) {
        LOGE("bgfx init failed!");
        return false;
    }
    
    // 6. 获取实际使用的渲染器类型
    bgfx::RendererType::Enum actualType = bgfx::getRendererType();
    LOGI("bgfx initialized with renderer: %d (%s)", actualType, bgfx::getRendererName(actualType));
    
    // 7. 启用调试文本（可选，用于显示 FPS 等信息）
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    
    // 8. 设置视图视口
    bgfx::setViewRect(0, 0, 0, (uint16_t)width, (uint16_t)height);
    
    // 9. 设置调试文本内容
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx triangle demo");
    bgfx::dbgTextPrintf(0, 2, 0x0f, "Press home button to exit.");
    
    // 9. 创建几何体和着色器
    createTriangle();
    createShader();
    
    // 标记为已初始化
    m_initialized = true;
    LOGI("BgfxRenderer initialized: %dx%d", width, height);
    
    return true;
}

// =============================================================================
// shutdown - 关闭渲染器
// =============================================================================

void BgfxRenderer::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    // 1. 销毁渲染资源（按照创建的逆序销毁）
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
        m_program.idx = UINT16_MAX;
    }
    
    if (bgfx::isValid(m_ibh)) {
        bgfx::destroy(m_ibh);
        m_ibh.idx = UINT16_MAX;
    }
    
    if (bgfx::isValid(m_vbh)) {
        bgfx::destroy(m_vbh);
        m_vbh.idx = UINT16_MAX;
    }
    
    // 2. 关闭 bgfx
    bgfx::shutdown();
    
    m_initialized = false;
    LOGI("BgfxRenderer shutdown complete.");
}

// =============================================================================
// render - 渲染一帧
// =============================================================================

void BgfxRenderer::render()
{
    if (!m_initialized) {
        return;
    }
    
    // 更新时间
    m_time += 0.016f;
    
    // 交替红绿蓝颜色
    static int colorIndex = 0;
    uint32_t colors[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };  // 红、绿、蓝 (ABGR)
    uint32_t clearColor = colors[colorIndex % 3];
    
    // 每秒切换一次颜色
    if (m_time > 1.0f) {
        colorIndex++;
        m_time = 0.0f;
    }
    
    LOGI("render: clearColor=0x%08X", clearColor);
    
    // 清除视图
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
    
    // 如果着色器有效，提交绘制
    if (bgfx::isValid(m_program)) {
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);
        bgfx::setState(BGFX_STATE_DEFAULT);
        bgfx::submit(0, m_program);
    } else {
        LOGI("Shader program invalid, skipping draw");
    }
    
    // 执行渲染
    bgfx::frame();
}

// =============================================================================
// createTriangle - 创建三角形几何体
// =============================================================================

void BgfxRenderer::createTriangle()
{
    // 创建顶点缓冲区
    const bgfx::Memory* mem = bgfx::makeRef(s_triangleVertices, sizeof(s_triangleVertices));
    m_vbh = bgfx::createVertexBuffer(mem, s_vertexLayout);
    
    // 创建索引缓冲区
    mem = bgfx::makeRef(s_triangleIndices, sizeof(s_triangleIndices));
    m_ibh = bgfx::createIndexBuffer(mem);
    
    LOGI("Triangle geometry created: %d vertices, %d indices",
         (int)(sizeof(s_triangleVertices) / sizeof(PosColorVertex)),
         (int)(sizeof(s_triangleIndices) / sizeof(uint16_t)));
}

// =============================================================================
// createShader - 创建着色器程序
// =============================================================================

void BgfxRenderer::createShader()
{
    // 检查渲染器类型
    bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
    LOGI("Current renderer type: %d (%s)", rendererType, bgfx::getRendererName(rendererType));
    
    // 使用 bgfx::copy 复制 shader 数据（bgfx 会管理数据生命周期）
    const bgfx::Memory* vsMem = bgfx::copy(s_vsMeshData, sizeof(s_vsMeshData));
    bgfx::ShaderHandle vsShader = bgfx::createShader(vsMem);
    
    if (!bgfx::isValid(vsShader)) {
        LOGE("Failed to create vertex shader!");
        return;
    }
    LOGI("Vertex shader created: handle=%d", vsShader.idx);
    
    // 创建片元着色器
    const bgfx::Memory* fsMem = bgfx::copy(s_fsMeshData, sizeof(s_fsMeshData));
    bgfx::ShaderHandle fsShader = bgfx::createShader(fsMem);
    
    if (!bgfx::isValid(fsShader)) {
        LOGE("Failed to create fragment shader!");
        bgfx::destroy(vsShader);
        return;
    }
    LOGI("Fragment shader created: handle=%d", fsShader.idx);
    
    // 创建程序句柄（true = 销毁着色器句柄）
    m_program = bgfx::createProgram(vsShader, fsShader, true);
    
    if (!bgfx::isValid(m_program)) {
        LOGE("Failed to create shader program!");
        return;
    }
    
    LOGI("Shader program created successfully: program=%d", m_program.idx);
}
