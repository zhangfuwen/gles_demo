//
// Created by zhangfuwen on 2022/3/22.
//

#ifndef TEST_GLES_H
#define TEST_GLES_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include "handycpp/file.h"

#include "handycpp/image.h"

#include "happly.h"

#include <cstdio>
#ifndef LOGI
#define LOGI(fmt, ...) FUN_INFO(fmt, ##__VA_ARGS__)
//#define LOGI(fmt, ...) printf(fmt "\n", ##__VA_ARGS__); fflush(stdout)
#endif

#include <ctime>
#include <memory>
#include <vector>

#define LOGE(fmt, ...) LOGI("-----------" fmt, ##__VA_ARGS__)

const char vertex_shader_fix[] = "#version 310 es\n"
                                 "layout(location = 1) in vec2 a_Position;\n"
                                 "uniform mat2 uRotation; \n"
                                 "out vec3 color; \n"
                                 "void main() {\n"
                                 "	gl_Position= vec4(uRotation *a_Position*2.0f, 0.0f, 1.0f);\n"
                                 "      color = vec3(a_Position, 1.0);\n"
                                 "}\n";

const char fragment_shader_simple[] = "#version 310 es\n"
                                      "precision mediump float;\n"
                                      "out vec4 FragColor;\n"
                                      "in vec3 color; \n"
                                      "void main(){\n"
                                      "	  FragColor = vec4(color ,1.0);\n"
                                      "}\n";

const float tableVerticesWithTriangles[] = {
    // Triangle1
    -0.5f,
    -0.5f,

    0.5f,
    0.5f,

    -0.5f,
    0.5f,
    // Triangle2
    -0.5f,
    -0.5f,

    0.5f,
    -0.5f,

    0.5f,
    0.5f,
};

class GLES {
public:
    GLuint program;

    int Init() {
        auto ret = LinkProgram(vertex_shader_fix, fragment_shader_simple);
        if (ret < 0) {
            return -1;
        }
        program = (GLuint)ret;
        return 0;
    }

    int Draw() {
        glUseProgram(program);
        auto ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to use program %d, error %d", program, ret);
            return -1;
        }

        GLuint aPositionLocation = glGetAttribLocation(program, "a_Position");
//        LOGI("aPositionLocation %d", aPositionLocation);
        glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, tableVerticesWithTriangles);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to set vertex attribute %d", ret);
            return -1;
        }
        glEnableVertexAttribArray(aPositionLocation);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to enable vertex attribute %d", ret);
            return -1;
        }
        static float angle = 0.0f;
        angle += 0.01f;
        if (angle > M_PI * 2.0f) {
            angle = 0.0f;
        }
//        float rotation[4] = {
//            cosf(angle),
//            sinf(angle),
//            sinf(angle),
//            cosf(angle),
//        };
        float rotation[4] = {
            1.0f, 0.0f,
                0.0f, 1.0f,
        };
        glUniformMatrix2fv(glGetUniformLocation(program, "uRotation"), 1, false, rotation);

        // draw something
        glDrawArrays(GL_TRIANGLES, 0, 6);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to draw %d", ret);
            return -1;
        }
//        LOGI("draw ok");
        return 0;
    }

    int Finish() {
        glDeleteProgram(program);
        return 0;
    }

    inline static int GetFramebufferInfo(bool read = true) {
        GLint val = -1;
        glGetFramebufferAttachmentParameteriv(
            read ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
            &val);
        auto ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to get attachment obj type: %d", ret);
            return -1;
        }
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
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to get attachment red size: %d", ret);
            return -1;
        }
        LOGI("red component size: %d", val);
        glGetFramebufferAttachmentParameteriv(
            read ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
            &val);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to get attachment obj name: %d, %s", ret);
            return -1;
        }
        LOGI("attachment name: %d", val);
        return 0;
    }

    inline static GLuint compileShader(GLenum type, const char *source) {
        GLint shaderLen = strlen(source);

        LOGI("compiling shader, type:%d, sourceLen:%d, source:%s", type, shaderLen, source);

        auto shader = glCreateShader(type);
        auto ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to create shader: %d", ret);
            return -1;
        }
        LOGI("created shader");

        glShaderSource(shader, 1, (const GLchar *const *)(&source), &shaderLen);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to attach shader source: %d", ret);
            return -1;
        }
        LOGI("set shader source");

        glCompileShader(shader);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to compile shader: %d", ret);
            return -1;
        }
        LOGI("compiled shader source");

        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) {
            LOGE("shader compilation failed");
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
            LOGE("errer log length: %d", maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> errorLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
            LOGE("errer log: %s", errorLog.data());
            glDeleteShader(shader); // Don't leak the shader.
            return -1;
        }
        LOGI("compile ok");
        return shader;
    }

    inline static int LinkProgram(const char *vshaderSource, const char *fshaderSource) {
        const char *vertex_shader = vshaderSource;
        const char *fragment_shader = fshaderSource;

        GLuint vertexShader;
        GLuint fragmentShader;
        if (vertexShader = compileShader(GL_VERTEX_SHADER, vertex_shader); vertexShader < 0) {
            LOGE("failed to compile shader");
            return -1;
        }
        if (fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragment_shader); fragmentShader < 0) {
            LOGE("failed to compile shader");
            return -1;
        }
        auto program = glCreateProgram();
        auto ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to create program: %d", ret);
            return -1;
        }

        glAttachShader(program, vertexShader);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to attach vertex shader %d", ret);
            return -1;
        }
        glAttachShader(program, fragmentShader);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to attach fragment shader %d", ret);
            return -1;
        }

        glLinkProgram(program);
        ret = glGetError();
        if (ret != GL_NO_ERROR) {
            LOGE("failed to link program %d", ret);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return -1;
        }

        GLint isCompiled = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) {
            LOGE("program link failed");
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            LOGE("errer log length: %d", maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> errorLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);
            LOGE("errer log: %s", errorLog.data());
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return -1;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        LOGI("linked program %d", program);
        return program;
    }

    /**
     * readPixels from framebuffer to a png file
     * @param filePath png file path
     * @param fbo fbo, default 0
     * @param src fbo source, default GL_BACK, other values maybe GL_COLOR_ATTACHMENTi
     * @param width with of the framebuffer, default 0 causes a framebuffer size query
     * @param height height of the framebuffer, default 0 causes a framebuffer size query
     * @return 0 on success, negative value on failure
     */
    static int readPixels(const char *filePath, GLuint fbo = 0, GLenum src = GL_BACK, int width = 0, int height = 0);

    inline static int GLES_CreateFBO(int width, int height) {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

//        GLuint renderbuffer = 0;
//        glGenRenderbuffers(1, &renderbuffer);
//        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
//        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
//        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);

        GLuint unity_tex;
        glGenTextures(1, &unity_tex);
        glBindTexture(GL_TEXTURE_2D, unity_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, unity_tex, 0);
        LOGI("unity_tex %d", unity_tex);

        GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (ret != GL_FRAMEBUFFER_COMPLETE) {
            LOGE("framebuffer incomplete");
            return -1;
        }
        return (int)fbo;
    }
};

#endif // TEST_GLES_H
