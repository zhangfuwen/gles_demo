//
// Created by zhangfuwen on 2022/2/16.
//

#define EGL_EGLEXT_PROTOTYPES 1
#define GL_GLEXT_PROTOTYPES 1
#define __ANDROID_API__ 29
#undef EGL_ANDROID_get_native_client_buffer
#undef GL_EXT_EGL_image_storage
#undef GL_EXT_EGL_image_storage

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include <cstdio>
#include <string>

#define CASE_STR(value)                                                                                                \
    case value:                                                                                                        \
        return #value;

inline std::string eglErrString(EGLint error) {
    switch (error) {
        CASE_STR(EGL_SUCCESS)
        CASE_STR(EGL_NOT_INITIALIZED)
        CASE_STR(EGL_BAD_ACCESS)
        CASE_STR(EGL_BAD_ALLOC)
        CASE_STR(EGL_BAD_ATTRIBUTE)
        CASE_STR(EGL_BAD_CONTEXT)
        CASE_STR(EGL_BAD_CONFIG)
        CASE_STR(EGL_BAD_CURRENT_SURFACE)
        CASE_STR(EGL_BAD_DISPLAY)
        CASE_STR(EGL_BAD_SURFACE)
        CASE_STR(EGL_BAD_MATCH)
        CASE_STR(EGL_BAD_PARAMETER)
        CASE_STR(EGL_BAD_NATIVE_PIXMAP)
        CASE_STR(EGL_BAD_NATIVE_WINDOW)
        CASE_STR(EGL_CONTEXT_LOST)
        default:
            std::string s = "Unknown ";
            s += std::to_string(error);
            return s;
    }
}
#undef CASE_STR

#define EGL_CHECK_ERROR(fmt, ...)                                                                                      \
    do {                                                                                                               \
        auto err = eglGetError();                                                                                      \
        if (err != EGL_SUCCESS) {                                                                                      \
            LOGE("line:%d: %s, " fmt, __LINE__, eglErrString(err).c_str(), ##__VA_ARGS__);                             \
        }                                                                                                              \
    } while (0)

#define EGL_CHECK_ERROR_RET(ret, fmt, ...)                                                                             \
    do {                                                                                                               \
        auto err = eglGetError();                                                                                      \
        if (err != EGL_SUCCESS) {                                                                                      \
            LOGE("line:%d: %s, " fmt, __LINE__, eglErrString(err).c_str(), ##__VA_ARGS__);                             \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)

class EGL {
public:
    void Finish() {
        eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(eglDisp, eglCtx);
        eglDestroySurface(eglDisp, eglSurface);
        eglTerminate(eglDisp);

        eglDisp = EGL_NO_DISPLAY;
        eglSurface = EGL_NO_SURFACE;
        eglCtx = EGL_NO_CONTEXT;
    }

    int Init(int width, int height, EGLNativeWindowType win = nullptr);

    EGLConfig GetConfig() {
        return eglConf;
    }
    EGLContext GetContext() {
        return eglCtx;
    }
    EGLSurface GetSurface() {
        return eglSurface;
    }

private:
    EGLConfig eglConf{EGL_NO_CONFIG_KHR};
    EGLSurface eglSurface{EGL_NO_SURFACE};
    EGLContext eglCtx{EGL_NO_CONTEXT};
    EGLDisplay eglDisp{EGL_NO_DISPLAY};
};

#define DEF_GLES_FUNC_POINTER(name) inline decltype(name) * pfn##name = nullptr

#define INIT_GLES_FUNC_POINTER(name) pfn##name = (decltype(name)*)eglGetProcAddress(#name)

#define LIST_GLES_POINTERS(macro) \
    macro(glCreateMemoryObjectsEXT); \
    macro(glMemoryObjectParameterivEXT); \
    macro(glImportMemoryFdEXT);   \
    macro(glTexStorageMem2DEXT)

LIST_GLES_POINTERS(DEF_GLES_FUNC_POINTER);