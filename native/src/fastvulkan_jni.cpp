#include <jni.h>
#include "vulkan_window.h"

extern "C" {

JNIEXPORT jlong JNICALL Java_fastvulkan_FastVulkanWindow_nCreateWindow(
    JNIEnv* env, jclass clazz, jstring title, jint width, jint height, jfloat r, jfloat g, jfloat b, jfloat a) {
    
    if (!title) return 0;
    const char* utf = env->GetStringUTFChars(title, nullptr);
    if (!utf) return 0;

    int len = MultiByteToWideChar(CP_UTF8, 0, utf, -1, nullptr, 0);
    std::vector<wchar_t> wbuf(len + 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf, -1, wbuf.data(), len);
    env->ReleaseStringUTFChars(title, utf);

    VulkanWindowContext* ctx = CreateVulkanWindow(wbuf.data(), width, height, r, g, b, a);
    return (jlong)ctx;
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nDestroyWindow(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (handle) {
        DestroyVulkanWindow((VulkanWindowContext*)handle);
    }
}

JNIEXPORT jboolean JNICALL Java_fastvulkan_FastVulkanWindow_nPollEvents(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (!handle) return JNI_FALSE;
    return PollWindowEvents((VulkanWindowContext*)handle) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nRenderAndPresent(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (handle) {
        RenderAndPresent((VulkanWindowContext*)handle);
    }
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetClearColor(
    JNIEnv* env, jclass clazz, jlong handle, jfloat r, jfloat g, jfloat b, jfloat a) {
    if (handle) {
        SetClearColor((VulkanWindowContext*)handle, r, g, b, a);
    }
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetTitle(
    JNIEnv* env, jclass clazz, jlong handle, jstring title) {
    if (handle && title) {
        const char* utf = env->GetStringUTFChars(title, nullptr);
        if (utf) {
            int len = MultiByteToWideChar(CP_UTF8, 0, utf, -1, nullptr, 0);
            if (len > 0) {
                std::vector<wchar_t> wbuf(len + 1, 0);
                MultiByteToWideChar(CP_UTF8, 0, utf, -1, wbuf.data(), len);
                auto ctx = (VulkanWindowContext*)handle;
                if (ctx && ctx->hwnd) {
                    SetWindowTextW(ctx->hwnd, wbuf.data());
                }
            }
            env->ReleaseStringUTFChars(title, utf);
        }
    }
}

JNIEXPORT jlong JNICALL Java_fastvulkan_FastVulkanWindow_nGetHWND(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (!handle) return 0;
    return (jlong)((VulkanWindowContext*)handle)->hwnd;
}

JNIEXPORT jint JNICALL Java_fastvulkan_FastVulkanWindow_nGetWidth(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (!handle) return 0;
    return (jint)GetWindowWidth((VulkanWindowContext*)handle);
}

JNIEXPORT jint JNICALL Java_fastvulkan_FastVulkanWindow_nGetHeight(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (!handle) return 0;
    return (jint)GetWindowHeight((VulkanWindowContext*)handle);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetLocation(
    JNIEnv* env, jclass clazz, jlong handle, jint x, jint y) {
    if (handle) SetWindowLocation((VulkanWindowContext*)handle, x, y);
}

JNIEXPORT jint JNICALL Java_fastvulkan_FastVulkanWindow_nGetX(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (!handle) return 0;
    return GetWindowX((VulkanWindowContext*)handle);
}

JNIEXPORT jint JNICALL Java_fastvulkan_FastVulkanWindow_nGetY(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (!handle) return 0;
    return GetWindowY((VulkanWindowContext*)handle);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetDimensions(
    JNIEnv* env, jclass clazz, jlong handle, jint width, jint height) {
    if (handle) SetWindowDimensions((VulkanWindowContext*)handle, width, height);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetBounds(
    JNIEnv* env, jclass clazz, jlong handle, jint x, jint y, jint width, jint height) {
    if (handle) SetWindowBounds((VulkanWindowContext*)handle, x, y, width, height);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nCenterOnScreen(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (handle) CenterWindowOnScreen((VulkanWindowContext*)handle);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetVisible(
    JNIEnv* env, jclass clazz, jlong handle, jboolean visible) {
    if (handle) SetWindowVisible((VulkanWindowContext*)handle, visible == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetResizable(
    JNIEnv* env, jclass clazz, jlong handle, jboolean resizable) {
    if (handle) SetWindowResizable((VulkanWindowContext*)handle, resizable == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetAlwaysOnTop(
    JNIEnv* env, jclass clazz, jlong handle, jboolean alwaysOnTop) {
    if (handle) SetWindowAlwaysOnTop((VulkanWindowContext*)handle, alwaysOnTop == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetFullscreen(
    JNIEnv* env, jclass clazz, jlong handle, jboolean fullscreen) {
    if (handle) SetWindowFullscreen((VulkanWindowContext*)handle, fullscreen == JNI_TRUE);
}

JNIEXPORT jboolean JNICALL Java_fastvulkan_FastVulkanWindow_nIsFullscreen(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (!handle) return JNI_FALSE;
    return IsWindowFullscreen((VulkanWindowContext*)handle) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nMinimize(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (handle) MinimizeWindow((VulkanWindowContext*)handle);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nMaximize(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (handle) MaximizeWindow((VulkanWindowContext*)handle);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nRestore(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (handle) RestoreWindow((VulkanWindowContext*)handle);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetMinSize(
    JNIEnv* env, jclass clazz, jlong handle, jint minW, jint minH) {
    if (handle) SetWindowMinSize((VulkanWindowContext*)handle, minW, minH);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetMaxSize(
    JNIEnv* env, jclass clazz, jlong handle, jint maxW, jint maxH) {
    if (handle) SetWindowMaxSize((VulkanWindowContext*)handle, maxW, maxH);
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nSetIcon(
    JNIEnv* env, jclass clazz, jlong handle, jintArray pixels, jint width, jint height) {
    if (handle && pixels && width > 0 && height > 0) {
        jint* rawPixels = env->GetIntArrayElements(pixels, nullptr);
        if (rawPixels) {
            SetWindowIcon((VulkanWindowContext*)handle, (const uint32_t*)rawPixels, width, height);
            env->ReleaseIntArrayElements(pixels, rawPixels, JNI_ABORT);
        }
    }
}

JNIEXPORT jlong JNICALL Java_fastvulkan_FastVulkanWindow_nCreateTexture(
    JNIEnv* env, jclass clazz, jlong handle, jintArray pixels, jint width, jint height, jboolean mipmaps) {
    if (!handle || !pixels || width <= 0 || height <= 0) return 0;

    jint* rawPixels = env->GetIntArrayElements(pixels, nullptr);
    VulkanTexture* tex = CreateTexture((VulkanWindowContext*)handle, (const uint32_t*)rawPixels, (uint32_t)width, (uint32_t)height, mipmaps == JNI_TRUE);
    env->ReleaseIntArrayElements(pixels, rawPixels, JNI_ABORT);

    return (jlong)tex;
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nDestroyTexture(
    JNIEnv* env, jclass clazz, jlong handle, jlong texHandle) {
    if (handle && texHandle) {
        DestroyTexture((VulkanWindowContext*)handle, (VulkanTexture*)texHandle);
    }
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nDrawImage(
    JNIEnv* env, jclass clazz, jlong handle, jlong texHandle,
    jfloat x, jfloat y, jfloat w, jfloat h,
    jfloat u0, jfloat v0, jfloat u1, jfloat v1,
    jfloat r, jfloat g, jfloat b, jfloat a) {
    if (handle && texHandle) {
        DrawImage((VulkanWindowContext*)handle, (VulkanTexture*)texHandle, x, y, w, h, u0, v0, u1, v1, r, g, b, a);
    }
}

JNIEXPORT void JNICALL Java_fastvulkan_FastVulkanWindow_nFlushBatch(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (handle) {
        FlushBatch((VulkanWindowContext*)handle);
    }
}

}
