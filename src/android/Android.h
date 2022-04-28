//
// Created by zhangfuwen on 2022/4/28.
//
#ifndef TEST_ANDROID_H
#define TEST_ANDROID_H
#define EGL_EGLEXT_PROTOTYPES 1
#define GL_GLEXT_PROTOTYPES 1
#define __ANDROID_API__ 29
#undef EGL_ANDROID_get_native_client_buffer
#undef GL_EXT_EGL_image_storage
#undef GL_EXT_EGL_image_storage

#include <memory>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>

#include <android/hardware_buffer.h>

#include "../common/common.h"

class AndroidAHardwareBuffer {
public:
    AndroidAHardwareBuffer(int width, int height);

    GLuint GetID() const { return m_tex; }
    ~AndroidAHardwareBuffer() {
        glDeleteTextures(1, &m_tex);
        AHardwareBuffer_release(m_aHardwareBuffer);
    }

public:
    static std::unique_ptr<char[]> readAHardwareBuffer(AHardwareBuffer *buf, int &stride, int32_t fence = -1);
    static AHardwareBuffer *allocAHardwareBuffer(uint32_t w, uint32_t h);

private:
    GLuint m_tex;
    AHardwareBuffer *m_aHardwareBuffer;
};

#endif // TEST_ANDROID_H
