//
// Created by zhangfuwen on 2022/4/28.
//

#include "Android.h"
#include "../common/EGL.h"
#include "../common/GLES.h"

std::unique_ptr<char[]>
AndroidAHardwareBuffer::readAHardwareBuffer(AHardwareBuffer *buf, int &stride, int32_t fence /* = -1 */) {
    void *ptr;
    int ret = AHardwareBuffer_lock(buf, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, fence, nullptr, &ptr);
    if (ret != 0) {
        LOGE("failed, %d", ret);
        return nullptr;
    }
    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(buf, &desc);
    stride = desc.stride;

    auto res = std::make_unique<char[]>(stride * desc.height * 4);
    memcpy(res.get(), ptr, stride * desc.height * 4);
    LOGI("width:%d, height:%d, stride %d", desc.width, desc.height, desc.stride);
    AHardwareBuffer_unlock(buf, nullptr);
    return res;
}

AHardwareBuffer *AndroidAHardwareBuffer::allocAHardwareBuffer(uint32_t w, uint32_t h) {
    // clang-format off
    AHardwareBuffer *hardwareBuffer = nullptr;
    AHardwareBuffer_Desc desc = {};
    desc.width = w;
    desc.height = h;
    desc.usage = AHARDWAREBUFFER_USAGE_CPU_READ_NEVER
                  | AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER
                    | AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER;
    desc.layers = 1;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    AHardwareBuffer_allocate(&desc, &hardwareBuffer);
    // clang-format on
    return hardwareBuffer;
}

AndroidAHardwareBuffer::AndroidAHardwareBuffer(int width, int height) {
    m_aHardwareBuffer = allocAHardwareBuffer(width, height);

    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_LINEAR_TILING_EXT);
    if(auto err = glGetError(); err != GL_NO_ERROR) {
        LOGE("failed to set gl tiling type");
    }


    EGLClientBuffer native_buffer = eglGetNativeClientBufferANDROID(m_aHardwareBuffer);
    // clang-format off
    EGLint attrs[] = {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_NONE};
    auto image = eglCreateImageKHR(
        eglGetCurrentDisplay(),
        EGL_NO_CONTEXT,
        EGL_NATIVE_BUFFER_ANDROID,
        native_buffer,
        attrs);
    // clang-format on

    EGL_CHECK_ERROR();

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
//    glEGLImageTargetTexStorageEXT(GL_TEXTURE_2D, static_cast<GLeglImageOES>(image), nullptr);
    GL_CHECK_ERROR();
}