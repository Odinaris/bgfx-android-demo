#ifndef BGFX_RENDERER_H
#define BGFX_RENDERER_H

// Note: Full bgfx integration requires including bgfx headers
// This stub header allows the project to compile for development.

#include <android/log.h>

#define LOG_TAG "BgfxRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class BgfxRenderer {
public:
    BgfxRenderer();
    ~BgfxRenderer();

    bool initialize(int width, int height, void* window);
    void shutdown();
    void render();

    bool isInitialized() const { return m_initialized; }

private:
    bool m_initialized;
    int m_width;
    int m_height;

    void createQuad();
    void createShader();
};

#endif // BGFX_RENDERER_H
