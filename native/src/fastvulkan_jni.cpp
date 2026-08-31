#include <jni.h>
#include "vulkan_window.h"

extern "C" {

JNIEXPORT jlong JNICALL Java_fastvulkan_FastVulkanWindow_nCreateWindow(
    JNIEnv* env, jclass clazz, jstring title, jint width, jint height, jfloat r, jfloat g, jfloat b, jfloat a) {
    
    if (!title) return 0;
    const jchar* rawChars = env->GetStringChars(title, nullptr);
    jsize len = env->GetStringLength(title);
    std::wstring wTitle((const wchar_t*)rawChars, len);
    env->ReleaseStringChars(title, rawChars);

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
        const jchar* rawChars = env->GetStringChars(title, nullptr);
        jsize len = env->GetStringLength(title);
        std::wstring wTitle((const wchar_t*)rawChars, len);
        env->ReleaseStringChars(title, rawChars);

        SetWindowTitle((VulkanWindowContext*)handle, wTitle.c_str());
    }
}

JNIEXPORT jlong JNICALL Java_fastvulkan_FastVulkanWindow_nGetHWND(
    JNIEnv* env, jclass clazz, jlong handle) {
    if (!handle) return 0;
    return (jlong)((VulkanWindowContext*)handle)->hwnd;
}

}
