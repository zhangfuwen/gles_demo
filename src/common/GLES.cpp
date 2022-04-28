//
// Created by zhangfuwen on 2022/3/22.
//

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "GLES.h"

#include "handycpp/file.h"
#include "handycpp/image.h"
#include "happly.h"
#include "common.h"

/**
 * ReadPixels from framebuffer to a png file
 * @param filePath png file path
 * @param fbo fbo, default 0
 * @param src fbo source, default GL_BACK, other values maybe GL_COLOR_ATTACHMENTi
 * @param width with of the framebuffer, default 0 causes a framebuffer size query
 * @param height height of the framebuffer, default 0 causes a framebuffer size query
 * @return 0 on success, negative value on failure
 */
int GLES::ReadPixels(const char *filePath, GLuint fbo, GLenum src, int width, int height) {
    GLint oldReadFbo = 0, oldPackBO = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFbo);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &oldPackBO);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    defer df([oldReadFbo]() { glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFbo); });

    if (width == 0 || height == 0) {
        glGetFramebufferParameteriv(GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, &width);
        glGetFramebufferParameteriv(GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, &height);
    }
    if (width == 0 || height == 0) {
        return -1;
    }

    FUN_INFO("file:%s, fbo:%d, src:%d, w:%d, h:%d", filePath, fbo, src, width, height);
    int dataSize = height * width * 4;
    GLuint pack_buffer_id;
    glGenBuffers(1, &pack_buffer_id);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pack_buffer_id);
    defer dp([oldPackBO]() { glBindBuffer(GL_PIXEL_PACK_BUFFER, oldPackBO); });
    glBufferData(GL_PIXEL_PACK_BUFFER, dataSize, nullptr, GL_STREAM_READ);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    auto *bufPtr = static_cast<GLubyte *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, dataSize, GL_MAP_READ_BIT));
    defer dm([]() { glUnmapBuffer(GL_PIXEL_PACK_BUFFER); });
    if (bufPtr) {
        if (auto ret = handycpp::image::saveRgbaToPng(filePath, bufPtr, width, height); ret < 0) {
            return ret;
        }
    }
    return 0;
}

/**
 * std::option<GLuint> CompileShader
 * @param type  GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * @param source
 * @return
 */
std::optional<GLuint> GLES::CompileShader(GLenum type, const char *source) {
    GLint shaderLen = strlen(source);

    LOGI("compiling shader, type:%d, sourceLen:%d, source:%s", type, shaderLen, source);

    auto shader = glCreateShader(type);
    GL_CHECK_ERROR_RET({}, "failed to create shader, type:%d, source:%s", type, source);
    LOGI("created shader %d", shader);

    glShaderSource(shader, 1, (const GLchar *const *)(&source), &shaderLen);
    GL_CHECK_ERROR_RET({}, "failed to attach shader source, shader:%d, source:%s", shader, source);
    LOGI("set shader source");

    glCompileShader(shader);
    //        GL_CHECK_ERROR_RET(-1, "failed to compile shader %d, source:%s", shader, source);
    LOGI("compiled shader source");

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
        glDeleteShader(shader); // Don't leak the shader.

        LOGE("failed to compile shader:%d, source:%s", shader, source);
        LOGE("error log: %s", errorLog.data());
        return -1;
    }

    LOGI("compile ok");
    return shader;
}

/**
 * static GLuint LinkProgram
 * @param vsSource
 * @param fsSource
 * @return linked program id
 */
std::optional<GLuint> GLES::LinkProgram(const char *vsSource, const char *fsSource) {
    const char *vSource = vsSource;
    const char *fSource = fsSource;

    std::optional<GLuint> vertexShader;
    std::optional<GLuint> fragmentShader;
    if (vertexShader = CompileShader(GL_VERTEX_SHADER, vSource); !vertexShader) {
        return -1;
    }
    if (fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fSource); !fragmentShader) {
        return -1;
    }
    defer shader_delete([&]() {
        glDeleteShader(vertexShader.value());
        glDeleteShader(fragmentShader.value());
    });

    auto program = glCreateProgram();
    GL_CHECK_ERROR_RET({}, "glCreateProgram failed");

    glAttachShader(program, vertexShader.value());
    GL_CHECK_ERROR_RET({}, "failed to attach vertex shader %d to program %d", vertexShader, program);

    glAttachShader(program, fragmentShader.value());
    GL_CHECK_ERROR_RET({}, "failed to attach fragment shader %d to program %d", fragmentShader, program);

    glLinkProgram(program);
    GL_CHECK_ERROR("glLinkProgram failed");

    GLint isCompiled = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<GLchar> errorLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);

        LOGE("glLinkProgram failed, error log: %s", errorLog.data());
        return {};
    }
    LOGI("linked program %d", program);
    return program;
}

/**
 * static GLuint CreateFBO
 * @param width
 * @param height
 * @param withDepthStencil = false, other wise a DEPTH_STENCIL renderbuffer is created
 * @param tex an optional texture could provided as GL_COLOR_ATTACHMENT0
 * @return fbo created
 */
std::optional<GLuint>
GLES::CreateFBO(int width, int height, bool withDepthStencil /* = false */, GLuint tex /* = 0 */) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    defer d([]() { glBindFramebuffer(GL_FRAMEBUFFER, 0); });

    if (withDepthStencil) {
        // create depth stencil attachment
        GLuint renderbuffer = 0;
        glGenRenderbuffers(1, &renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        defer dr([]() { glBindRenderbuffer(GL_RENDERBUFFER, 0); });
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        // attach depth stencil attachment
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
    }

    // create color attachment
    GLuint colorTex;
    if (tex != 0) {
        colorTex = tex;
    } else {
        glGenTextures(1, &colorTex);
        glBindTexture(GL_TEXTURE_2D, colorTex);
        defer dt([]() { glBindTexture(GL_TEXTURE_2D, 0); });
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        GL_CHECK_ERROR_RET({}, "glTexImage2D failed");
    }

    // attach color attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
    GL_CHECK_ERROR_RET({}, "glFramebufferTexture2D faild");

    // check framebuffer completeness
    GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (ret != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("framebuffer incomplete");
        return -1;
    }
    return fbo;
}

/**
 * print framebuffer into
 * @param read read or draw framebuffer
 * @return
 */
int GLES::GetFramebufferInfo(bool read /* = true */) {
    GLint val = -1;
    glGetFramebufferAttachmentParameteriv(
        read ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
        &val);
    GL_CHECK_ERROR_RET(-1, "failed to get GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE for GL_COLOR_ATTACHMENT0");

    switch (val) {
        case GL_NONE:
            LOGE("attachment type: none");
            break;
        case GL_TEXTURE:
            LOGI("attachment type: texture");
            break;
        case GL_FRAMEBUFFER_DEFAULT:
            LOGI("attachment type: default");
            break;
        case GL_RENDERBUFFER:
            LOGI("attachment type: renderbuffer");
            break;
        default:
            LOGE("attachment type: unknown %d(0x%x)", val, val);
            break;
    }

    glGetFramebufferAttachmentParameteriv(
        read ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,
        &val);
    GL_CHECK_ERROR_RET(-1, "failed to get GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE for GL_COLOR_ATTACHMENT0");
    LOGI("red component size: %d", val);

    glGetFramebufferAttachmentParameteriv(
        read ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
        &val);
    GL_CHECK_ERROR_RET(-1, "failed to get FRAMEBUFFER_ATTACHMEN_OBJECT_NAME of GL_COLOR_ATTACHMENT0");
    LOGI("attachment name: %d", val);
    return 0;
}

int GLES::Draw() const {
    glUseProgram(program);
    GL_CHECK_ERROR_RET(-1, "glUseProgram %d", program);

    GLuint aPositionLocation = glGetAttribLocation(program, "a_Position");
    glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, tableVerticesWithTriangles);
    GL_CHECK_ERROR_RET(-1, "glVertexAttribPointer");

    glEnableVertexAttribArray(aPositionLocation);
    GL_CHECK_ERROR_RET(-1, "glEnableVertexAttribArray");

    static float angle = 0.0f;
    angle += 0.01f;
    if (angle > M_PI * 2.0f) {
        angle = 0.0f;
    }

    // clang-format off
#if 0
        float rotation[4] = {
            cosf(angle), -sinf(angle),
            sinf(angle), cosf(angle),
        };
#else
        float rotation[4] = {
            1.0f, 0.0f,
            0.0f, 1.0f,
        };
#endif
    // clang-format on

    glUniformMatrix2fv(glGetUniformLocation(program, "uRotation"), 1, false, rotation);

    // draw something
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL_CHECK_ERROR_RET(-1, "glDrawArrays failed");
    return 0;
}
#if 0
void drawBunny() {
    glUseProgram(program);

    // Construct the data object by reading from file
    happly::PLYData plyIn("/data/bun_zipper.ply");

    // Get mesh-style data from the object
    std::vector<std::array<double, 3>> vPos = plyIn.getVertexPositions();
    std::vector<std::vector<size_t>> fInd = plyIn.getFaceIndices<size_t>();
    FUN_INFO("fInd.size %d", fInd.size());
//    for(int i = 0; i< fInd.size();i++) {
//        FUN_INFO("fInd[i].size %d", fInd[i].size());
//    }

    std::vector<std::array<int, 3>> fIndInt(fInd.size());
    for(int i = 0; i< fInd.size();i++) {
        fIndInt[i][0] = fInd[i][0];
        fIndInt[i][1] = fInd[i][1];
        fIndInt[i][2] = fInd[i][2];
    }


    std::vector<std::array<float, 3>> vPosFloat(vPos.size());
    for(int i = 0; i< vPos.size();i++) {
        vPosFloat[i][0] = vPos[i][0];
        vPosFloat[i][1] = vPos[i][1];
        vPosFloat[i][2] = vPos[i][2];
    }

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);

    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vPosFloat.size()*3*sizeof(float), &vPosFloat[0][0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, fIndInt.size()*3*sizeof(int), &fIndInt[0][0] , GL_STATIC_DRAW);

    GLuint aPositionLocation = glGetAttribLocation(program, "a_Position");
    FUN_INFO("aPosition %d", aPositionLocation);
    glBindVertexArray(VAO);

    glVertexAttribPointer(aPositionLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(aPositionLocation);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, fIndInt.size()*3, GL_UNSIGNED_INT, 0);

    // draw something
    glDrawElements(GL_TRIANGLE_STRIP, fInd.size(), GL_UNSIGNED_INT, fIndInt.data());
    glBindVertexArray(0);

    vPosFloat[0][0] = 0;
    fIndInt[0][0] = 0;
}
#endif
