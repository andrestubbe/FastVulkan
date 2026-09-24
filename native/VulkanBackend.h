#pragma once
#include <cstdint>

#ifdef _WIN32
#define FASTVK_API __declspec(dllexport)
#else
#define FASTVK_API __attribute__((visibility("default")))
#endif

extern "C" {

FASTVK_API int64_t fastvk_create(int64_t hwnd, int32_t w, int32_t h);
FASTVK_API void    fastvk_resize(int64_t handle, int32_t w, int32_t h);

FASTVK_API void    fastvk_begin_frame(int64_t handle);
FASTVK_API void    fastvk_clear(int64_t handle, float r, float g, float b, float a);

FASTVK_API void    fastvk_set_viewport(int64_t handle, int32_t x, int32_t y, int32_t w, int32_t h);
FASTVK_API void    fastvk_set_projection(int64_t handle, const float* matrix16);
FASTVK_API void    fastvk_set_blend_mode(int64_t handle, int32_t mode);

FASTVK_API void    fastvk_draw_triangles(int64_t handle,
                                         const float* vbData, int32_t vertexCount,
                                         const int32_t* ibData, int32_t indexCount,
                                         int64_t textureHandle);

FASTVK_API int64_t fastvk_create_texture(int64_t handle, int32_t w, int32_t h, const void* pixels);
FASTVK_API void    fastvk_update_texture(int64_t handle, int64_t texHandle, const void* pixels);
FASTVK_API void    fastvk_destroy_texture(int64_t handle, int64_t texHandle);

FASTVK_API void    fastvk_end_frame(int64_t handle);
FASTVK_API void    fastvk_present(int64_t handle);
FASTVK_API void    fastvk_destroy(int64_t handle);

}

