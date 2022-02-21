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

#include <android/hardware_buffer.h>

#include "handycpp/file.h"
#include "handycpp/image.h"

#include "happly.h"

#include <cstdio>
#define LOGI(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#include <android/log.h>
#include <ctime>
#include <memory>
#include <vector>

#define LOGE LOGI

#define VIEW_PORT_WIDTH 1921
#define VIEW_PORT_HEIGHT 1081

#define LOG_TAG "offscreen"
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static EGLConfig eglConf;
static EGLSurface eglSurface;
static EGLContext eglCtx;
static EGLDisplay eglDisp;
const char vertex_shader_fix[] = "attribute vec4 a_Position;\n"
                                 "void main() {\n"
                                 "	gl_Position=a_Position;\n"
                                 "}\n";

const char fragment_shader_simple[] = "precision mediump float;\n"
                                      "void main(){\n"
                                      "	gl_FragColor = vec4(1.0,1.0,0.0,1.0);\n"
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

void init() {
    // EGL config attributes
    const EGLint confAttr[] = {
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT, // very important!
        EGL_SURFACE_TYPE,
        EGL_PBUFFER_BIT, // EGL_WINDOW_BIT EGL_PBUFFER_BIT we will create a
                         // pixelbuffer surface
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8, // if you need the alpha channel
        EGL_DEPTH_SIZE,
        8, // if you need the depth buffer
        EGL_STENCIL_SIZE,
        8,
        EGL_NONE};
    // EGL context attributes
    const EGLint ctxAttr[] = {
        EGL_CONTEXT_CLIENT_VERSION,
        2, // very important!
        EGL_NONE};
    // surface attributes
    // the surface size is set to the input frame size
    const EGLint surfaceAttr[] = {EGL_WIDTH, VIEW_PORT_WIDTH, EGL_HEIGHT, VIEW_PORT_HEIGHT, EGL_NONE};
    EGLint eglMajVers, eglMinVers;
    EGLint numConfigs;

    eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisp == EGL_NO_DISPLAY) {
        // Unable to open connection to local windowing system
        LOGI("Unable to open connection to local windowing system");
    }
    if (!eglInitialize(eglDisp, &eglMajVers, &eglMinVers)) {
        // Unable to initialize EGL. Handle and recover
        LOGI("Unable to initialize EGL");
    }
    LOGI("EGL init with version %d.%d", eglMajVers, eglMinVers);
    // choose the first config, i.e. best config
    if (!eglChooseConfig(eglDisp, confAttr, &eglConf, 1, &numConfigs)) {
        LOGI("some config is wrong");
    } else {
        LOGI("all configs is OK");
    }
    // create a pixelbuffer surface
    eglSurface = eglCreatePbufferSurface(eglDisp, eglConf, surfaceAttr);
    if (eglSurface == EGL_NO_SURFACE) {
        switch (eglGetError()) {
            case EGL_BAD_ALLOC:
                // Not enough resources available. Handle and recover
                LOGI("Not enough resources available");
                break;
            case EGL_BAD_CONFIG:
                // Verify that provided EGLConfig is valid
                LOGI("provided EGLConfig is invalid");
                break;
            case EGL_BAD_PARAMETER:
                // Verify that the EGL_WIDTH and EGL_HEIGHT are
                // non-negative values
                LOGI("provided EGL_WIDTH and EGL_HEIGHT is invalid");
                break;
            case EGL_BAD_MATCH:
                // Check window and EGLConfig attributes to determine
                // compatibility and pbuffer-texture parameters
                LOGI("Check window and EGLConfig attributes");
                break;
        }
    }
    eglCtx = eglCreateContext(eglDisp, eglConf, EGL_NO_CONTEXT, ctxAttr);
    if (eglCtx == EGL_NO_CONTEXT) {
        EGLint error = eglGetError();
        if (error == EGL_BAD_CONFIG) {
            // Handle error and recover
            LOGI("EGL_BAD_CONFIG");
        }
    }
    if (!eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx)) {
        LOGI("MakeCurrent failed");
    }
    LOGI("initialize success!");
}

GLuint program;

AHardwareBuffer *allocAHardwareBuffer();

bool compileShader(GLuint shader) {
    glCompileShader(shader);
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        return false;
    }
    return true;
}

void prepare() {
    const char *vertex_shader = vertex_shader_fix;
    const char *fragment_shader = fragment_shader_simple;
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glClearColor(0.0, 0.0, 0.0, 0.0);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LESS);
//    glCullFace(GL_BACK);
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex_shader, NULL);
    if (!compileShader(vertexShader)) {
        LOGE("failed to compile shader");
        return;
    }
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment_shader, NULL);
    glCompileShader(fragmentShader);
    if (!compileShader(fragmentShader)) {
        LOGE("failed to compile shader");
        return;
    }
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
}

void draw() {
    glUseProgram(program);
    glViewport(0, 0, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint aPositionLocation = glGetAttribLocation(program, "a_Position");
    glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, tableVerticesWithTriangles);
    glEnableVertexAttribArray(aPositionLocation);

    // draw something
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glViewport(0, 0, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    eglSwapBuffers(eglDisp, eglSurface);
}

void drawBunny() {
    glUseProgram(program);
    glViewport(0, 0, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
    glViewport(0, 0, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    auto err = glGetError();
    if (err != GL_NO_ERROR) {
        FUN_ERROR("failed 0x%04X", err);
    }
    glDrawElements(GL_TRIANGLE_STRIP, fInd.size(), GL_UNSIGNED_INT, fIndInt.data());
    glViewport(0, 0, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    glBindVertexArray(0);

    vPosFloat[0][0] = 0;
    fIndInt[0][0] = 0;
//    eglSwapBuffers(eglDisp, eglSurface);
}

void fini() {
    eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisp, eglCtx);
    eglDestroySurface(eglDisp, eglSurface);
    eglTerminate(eglDisp);

    eglDisp = EGL_NO_DISPLAY;
    eglSurface = EGL_NO_SURFACE;
    eglCtx = EGL_NO_CONTEXT;
}

AHardwareBuffer *allocAHardwareBuffer(uint32_t w, uint32_t h) {
    AHardwareBuffer *hardwareBuffer = nullptr;
    AHardwareBuffer_Desc desc = {};
    desc.width = w;
    desc.height = h;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;
    desc.layers = 1;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    AHardwareBuffer_allocate(&desc, &hardwareBuffer);
    return hardwareBuffer;
}

std::unique_ptr<char[]> readAhardwareBuffer(AHardwareBuffer *buf, int &stride, int32_t fence = -1) {
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

int main() {
    // 1.  runtime get ahardwarebuffer from compositor
    AHardwareBuffer *hardwareBuffer = allocAHardwareBuffer(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);

    init();

    // 2. unity create texture
    GLuint unity_tex;
    glGenTextures(1, &unity_tex);
    glBindTexture(GL_TEXTURE_2D, unity_tex);
    // unity render to this texture

    // 3.  associate with texture with ahardwarebuffer
    EGLClientBuffer native_buffer = nullptr;
    native_buffer = eglGetNativeClientBufferANDROID(hardwareBuffer);
    EGLint attrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE, EGL_NONE};
    auto image =
        eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, native_buffer, attrs);
    glEGLImageTargetTexStorageEXT(GL_TEXTURE_2D, image, nullptr);
    // glEGLImageTargetTexStorageEXT replace unity_tex's backing store with
    // native_buffer's, which causes unity_tex's back store freed and orphan'd

    // 4. unity render to unity_tex again and again...
    timeval t1, t2;
    prepare();

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GLuint renderbuffer = 0;
    glGenRenderbuffers(1, &renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, unity_tex, 0);

    GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (ret != GL_FRAMEBUFFER_COMPLETE) {
        FUN_ERROR("framebuffer incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gettimeofday(&t1, nullptr);
    drawBunny();
    glFinish();
    gettimeofday(&t2, nullptr);
    LOGI("draw time(fbo 0) used: %ld us", (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec);
    void *out = malloc(VIEW_PORT_HEIGHT * VIEW_PORT_WIDTH * 4);
    glReadPixels(0, 0, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, out);
    handycpp::image::writeBmp("/data/1.bmp", (unsigned char *)out, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT, 4);

    // draw on ahardwarebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gettimeofday(&t1, nullptr);
    drawBunny();
    glFinish();
    gettimeofday(&t2, nullptr);
    LOGI("draw time(ahardwarebuffer) used: %ld us", (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec);

    int stride;
    auto buf = readAhardwareBuffer(hardwareBuffer, stride);
    if (buf == nullptr) {
        FUN_DEBUG("nullptr");
    }
    handycpp::image::writeBmp("/data/2.bmp", (unsigned char *)buf.get(), stride, VIEW_PORT_HEIGHT, 4);

    fini();

    return 0;
}
