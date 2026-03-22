#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "bgfx_renderer.h"

static BgfxRenderer* s_renderer = nullptr;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_odinaris_bgfx_1android_1demo_MainActivity_initBgfx(JNIEnv* env, jobject thiz, jint width, jint height, jobject surface)
{
    if (s_renderer != nullptr) {
        delete s_renderer;
    }

    s_renderer = new BgfxRenderer();

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    bool success = s_renderer->initialize(width, height, window);

    if (!success) {
        delete s_renderer;
        s_renderer = nullptr;
    }

    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_odinaris_bgfx_1android_1demo_MainActivity_shutdownBgfx(JNIEnv* env, jobject thiz)
{
    if (s_renderer != nullptr) {
        delete s_renderer;
        s_renderer = nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_odinaris_bgfx_1android_1demo_MainActivity_renderBgfxFrame(JNIEnv* env, jobject thiz)
{
    if (s_renderer != nullptr) {
        s_renderer->render();
    }
}

JNIEXPORT jboolean JNICALL
Java_com_odinaris_bgfx_1android_1demo_MainActivity_isBgfxInitialized(JNIEnv* env, jobject thiz)
{
    return (s_renderer != nullptr && s_renderer->isInitialized()) ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"
