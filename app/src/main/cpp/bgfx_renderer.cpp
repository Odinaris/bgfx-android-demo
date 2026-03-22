#include "bgfx_renderer.h"
#include <bgfx/bgfx.h>
#include <bx/bx.h>
#include <bx/fpumath.h>
#include <memory>

namespace
{
    struct PosColorVertex
    {
        float x;
        float y;
        float z;
        uint32_t abgr;
    };

    static PosColorVertex s_cubeVertices[] =
    {
        { -1.0f, -1.0f,  1.0f, 0xff000000 },
        {  1.0f, -1.0f,  1.0f, 0xff0000ff },
        { -1.0f,  1.0f,  1.0f, 0xff00ff00 },
        {  1.0f,  1.0f,  1.0f, 0xff00ffff },
        { -1.0f, -1.0f, -1.0f, 0xffff0000 },
        {  1.0f, -1.0f, -1.0f, 0xffff00ff },
        { -1.0f,  1.0f, -1.0f, 0xffffff00 },
        {  1.0f,  1.0f, -1.0f, 0xffffffff },
    };

    static const uint16_t s_cubeIndices[] =
    {
        0, 1, 2,      // 0
        1, 3, 2,      // 1
        4, 6, 5,      // 2
        5, 6, 7,      // 3
        0, 2, 4,      // 4
        2, 6, 4,      // 5
        1, 5, 3,      // 6
        3, 5, 7,      // 7
        0, 4, 1,      // 8
        1, 4, 5,      // 9
        2, 3, 6,      // 10
        3, 7, 6,      // 11
    };
}

BgfxRenderer::BgfxRenderer()
    : m_initialized(false)
    , m_width(0)
    , m_height(0)
    , BGFX_INVALID_HANDLE
    , BGFX_INVALID_HANDLE
    , BGFX_INVALID_HANDLE
    , BGFX_INVALID_HANDLE
    , BGFX_INVALID_HANDLE
{
}

BgfxRenderer::~BgfxRenderer()
{
    shutdown();
}

bool BgfxRenderer::initialize(int width, int height, void* window)
{
    if (m_initialized) {
        LOGE("Already initialized");
        return false;
    }

    m_width = width;
    m_height = height;

    // Initialize bgfx for Android
    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGLES;
    init.resolution.width = (uint32_t)width;
    init.resolution.height = (uint32_t)height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    // Set platform data for Android
    init.platformData.ndt = NULL;
    init.platformData.nwh = window;
    init.platformData.context = NULL;
    init.platformData.backBuffer = NULL;
    init.platformData.backBufferDS = NULL;

    if (!bgfx::init(init)) {
        LOGE("bgfx init failed");
        return false;
    }

    // Set debug flags
    bgfx::setDebug(BGFX_DEBUG_TEXT);

    // Set view rect for the first view
    bgfx::setViewRect(0, 0, bgfx::BackbufferRatio::Equal);

    // Create cube geometry
    createQuad();

    m_initialized = true;
    LOGI("BgfxRenderer initialized: %dx%d", width, height);

    return true;
}

void BgfxRenderer::shutdown()
{
    if (!m_initialized) {
        return;
    }

    if (bgfx::isValid(m_vbh))
    {
        bgfx::destroy(m_vbh);
    }

    if (bgfx::isValid(m_ibh))
    {
        bgfx::destroy(m_ibh);
    }

    if (bgfx::isValid(m_program))
    {
        bgfx::destroy(m_program);
    }

    if (bgfx::isValid(m_textureUniform))
    {
        bgfx::destroy(m_textureUniform);
    }

    if (bgfx::isValid(m_texture))
    {
        bgfx::destroy(m_texture);
    }

    bgfx::shutdown();
    m_initialized = false;
    LOGI("BgfxRenderer shutdown");
}

void BgfxRenderer::render()
{
    if (!m_initialized) {
        return;
    }

    // Advance to next frame
    bgfx::frame();
}

void BgfxRenderer::createQuad()
{
    // Create static vertex buffer
    bgfx::Memory* mem = bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices));
    m_vbh = bgfx::createVertexBuffer(mem, PosColorVertex::layout);

    // Create static index buffer
    mem = bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices));
    m_ibh = bgfx::createIndexBuffer(mem);

    // Create color uniform
    m_textureUniform = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);

    // Create program - using basic vertex/fragment shader
    // For a real app, you would load pre-compiled shaders
    // Here we use bgfx's embedded shaders for demonstration
    const bgfx::EmbeddedShader s_embeddedShaders[] =
    {
        BGFX_EMBEDDED_SHADER(vs_mesh),
        BGFX_EMBEDDED_SHADER(fs_mesh),
        BGFX_EMBEDDED_SHADER_END()
    };

    bgfx::RendererType::Enum type = bgfx::getRendererType();
    m_program = bgfx::createEmbeddedProgram(s_embeddedShaders, type);
}

void BgfxRenderer::createShader()
{
    // Placeholder for custom shader loading
    // In a real application, you would compile .sc files using shaderc
}
