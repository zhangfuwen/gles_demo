//
// Created by zhangfuwen on 2022/3/22.
//

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "GLES.h"

/**
 * readPixels from framebuffer to a png file
 * @param filePath png file path
 * @param fbo fbo, default 0
 * @param src fbo source, default GL_BACK, other values maybe GL_COLOR_ATTACHMENTi
 * @param width with of the framebuffer, default 0 causes a framebuffer size query
 * @param height height of the framebuffer, default 0 causes a framebuffer size query
 * @return 0 on success, negative value on failure
 */
int GLES::readPixels(const char *filePath, GLuint fbo, GLenum src, int width, int height) {
    GLint oldReadFbo = 0, oldPackBO = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFbo);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &oldPackBO);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

    if (width == 0 || height == 0) {
        glGetFramebufferParameteriv(GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, &width);
        glGetFramebufferParameteriv(GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, &height);
    }
    if (width == 0 || height == 0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFbo);
        return -1;
    }
    FUN_INFO("file:%s, fbo:%d, src:%d, w:%d, h:%d", filePath, fbo, src, width, height);
    int dataSize = height * width * 4;
    GLuint pack_buffer_id;
    glGenBuffers(1, &pack_buffer_id);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pack_buffer_id);
    glBufferData(GL_PIXEL_PACK_BUFFER, dataSize, nullptr, GL_STREAM_READ);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    auto *bufPtr = static_cast<GLubyte *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, dataSize, GL_MAP_READ_BIT));
    if (bufPtr) {
        if (auto ret = handycpp::image::saveRgbaToPng(filePath, bufPtr, width, height); ret < 0) {
            return ret;
        }
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, oldPackBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFbo);
    return 0;
}

