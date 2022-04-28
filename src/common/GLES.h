//
// Created by zhangfuwen on 2022/3/22.
//
#ifndef TEST_GLES_H
#define TEST_GLES_H

#include <cstdio>
#include <ctime>
#include <memory>
#include <optional>
#include <vector>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

inline const char *glErrString(GLenum error) {
    const static char *glErrorString[] = {
        "GL_INVALID_ENUM",                  // 0x0500
        "GL_INVALID_VALUE",                 // 0x0501
        "GL_INVALID_OPERATION",             // 0x0502
        "GL_STACK_OVERFLOW",                // 0x0503
        "GL_STACK_UNDERFLOW",               // 0x0504
        "GL_OUT_OF_MEMORY",                 // 0x0505
        "GL_INVALID_FRAMEBUFFER_OPERATION", // 0x0506
        "GL_CONTEXT_LOST",                  // 0x0507
    };
    const static char *unk = "Unknown error";
    if (error >= 0x500 && error <= 0x507) {
        return glErrorString[error - 0x500];
    } else {
        return unk;
    }
}

#define GL_CHECK_ERROR(fmt, ...)                                                                                       \
    do {                                                                                                               \
        auto err = glGetError();                                                                                       \
        if (err != GL_NO_ERROR) {                                                                                      \
            LOGE("l%s, " fmt, glErrString(err), ##__VA_ARGS__);                                      \
        }                                                                                                              \
    } while (0)

#define GL_CHECK_ERROR_RET(ret, fmt, ...)                                                                              \
    do {                                                                                                               \
        auto err = glGetError();                                                                                       \
        if (err != GL_NO_ERROR) {                                                                                      \
            LOGE("%s, " fmt, glErrString(err), ##__VA_ARGS__);                                      \
            return ret;                                                                                                \
        }                                                                                                              \
    } while (0)

const char vertexShaderSource[] = R"(
#version 310 es
layout(location = 1) in vec2 a_Position;
uniform mat2 uRotation;
out vec3 color;
void main() {
    gl_Position= vec4(uRotation *a_Position*2.0f, 0.0f, 1.0f);
    color = vec3(a_Position, 1.0);
}
)";

const char fragmentShaderSource[] = R"(
#version 310 es
precision mediump float;
out vec4 FragColor;
in vec3 color;
void main(){
    FragColor = vec4(color ,1.0);
}
)";

// clang-format off
const float tableVerticesWithTriangles[] = {
    // Triangle1
    -0.5f, -0.5f, // bottom-left 0
    0.5f, 0.5f,   // top-right   2
    -0.5f, 0.5f,  // top-left    3
    // Triangle2
    -0.5f, -0.5f, // bottom-left  0
    0.5f, -0.5f,  // bottom-right 1
    0.5f, 0.5f, // top-right    2
};
// clang-format on

class GLES {
public:
    GLuint program;

    int Init() {
        if(auto ret = LinkProgram(vertexShaderSource, fragmentShaderSource); ret) {
            program = ret.value();
            return 0;
        }
        return -1;
    }

    int Draw() const;

    int Finish() const {
        glDeleteProgram(program);
        return 0;
    }

public:
    static std::optional<GLuint> CompileShader(GLenum type, const char *source);
    static std::optional<GLuint> LinkProgram(const char *vsSource, const char *fsSource);
    static std::optional<GLuint> CreateFBO(int width, int height, bool withDepthStencil = false, GLuint tex = 0);
    static int ReadPixels(const char *filePath, GLuint fbo = 0, GLenum src = GL_BACK, int width = 0, int height = 0);
    static int GetFramebufferInfo(bool read = true);
};

#endif // TEST_GLES_H
