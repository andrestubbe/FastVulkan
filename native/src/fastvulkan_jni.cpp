#include <jni.h>
#include "vulkan_window.h"

extern "C" {

JNIEXPORT jlong JNICALL Java_fastvulkan_FastVulkanWindow_nCreateWindow(
    JNIEnv* env, jclass clazz, jstring title, jint width, jint height, jfloat r, jfloat g, jfloat b, jfloat a) {
    
    if (!title) return 0;
    const jchar* chars = env->GetStringChars(title, nullptr);
    jsize len = env->GetStringLength(title);
    std::wstring wTitle;
    wTitle.resize(len);
    for (jsize i = 0; i < len; i++) {
        wTitle[i] = (wchar_t)chars[i];
    }
    env->ReleaseStringChars(title, chars);

    VulkanWindowContext* ctx = CreateVulkanWindow(wTitle.c_str(), width, height, r, g, b, a);
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
            auto ctx = (VulkanWindowContext*)handle;
            if (ctx && ctx->hwnd) {
                SetWindowTextA(ctx->hwnd, utf);
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
