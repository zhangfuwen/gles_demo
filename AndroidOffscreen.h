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
#include <GLES3/gl32.h>

#include <android/hardware_buffer.h>

#include "handycpp/file.h"
#include "handycpp/image.h"

#include "happly.h"

#include <cstdio>
#define LOGI(fmt, ...)                                                                                                 \
    printf(fmt "\n", ##__VA_ARGS__);                                                                                   \
    fflush(stdout)
//#define LOGI(fmt, ...) FUN_INFO(fmt, ##__VA_ARGS__)
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <ctime>
#include <memory>
#include <vector>

#define LOGE(fmt, ...) LOGI("-----------" fmt, ##__VA_ARGS__)

#define LOG_TAG "offscreen"
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

class AndroidOffscreen {
public:
    void eglFinish() {
        eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(eglDisp, eglCtx);
        eglDestroySurface(eglDisp, eglSurface);
        eglTerminate(eglDisp);

        eglDisp = EGL_NO_DISPLAY;
        eglSurface = EGL_NO_SURFACE;
        eglCtx = EGL_NO_CONTEXT;
    }

    int eglInit(int width, int height) {
        // EGL config attributes
        const EGLint confAttr[] = {
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES3_BIT, // very important!
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
            3, // very important!
            EGL_NONE};
        // surface attributes
        // the surface size is set to the input frame size
        const EGLint surfaceAttr[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
        EGLint eglMajVers, eglMinVers;
        EGLint numConfigs;

        eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (eglDisp == EGL_NO_DISPLAY) {
            // Unable to open connection to local windowing system
            LOGE("Unable to open connection to local windowing system");
            return -1;
        }
        if (!eglInitialize(eglDisp, &eglMajVers, &eglMinVers)) {
            // Unable to initialize EGL. Handle and recover
            LOGE("Unable to initialize EGL");
            return -1;
        }
        LOGI("EGL eglInit with version %d.%d", eglMajVers, eglMinVers);
        // choose the first config, i.e. best config
        if (!eglChooseConfig(eglDisp, confAttr, &eglConf, 1, &numConfigs)) {
            LOGE("some config is wrong");
            return -1;
        } else {
            LOGI("all configs is OK");
        }
        // create a pixelbuffer surface
        eglSurface = eglCreatePbufferSurface(eglDisp, eglConf, surfaceAttr);
        if (eglSurface == EGL_NO_SURFACE) {
            switch (eglGetError()) {
                case EGL_BAD_ALLOC:
                    // Not enough resources available. Handle and recover
                    LOGE("Not enough resources available");
                    break;
                case EGL_BAD_CONFIG:
                    // Verify that provided EGLConfig is valid
                    LOGE("provided EGLConfig is invalid");
                    break;
                case EGL_BAD_PARAMETER:
                    // Verify that the EGL_WIDTH and EGL_HEIGHT are
                    // non-negative values
                    LOGE("provided EGL_WIDTH and EGL_HEIGHT is invalid");
                    break;
                case EGL_BAD_MATCH:
                    // Check window and EGLConfig attributes to determine
                    // compatibility and pbuffer-texture parameters
                    LOGE("Check window and EGLConfig attributes");
                    break;
            }
        }
        if (eglSurface == EGL_NO_SURFACE) {
            return -1;
        }
        eglCtx = eglCreateContext(eglDisp, eglConf, EGL_NO_CONTEXT, ctxAttr);
        if (eglCtx == EGL_NO_CONTEXT) {
            EGLint error = eglGetError();
            if (error == EGL_BAD_CONFIG) {
                // Handle error and recover
                LOGE("EGL_BAD_CONFIG");
            }
        }
        if (eglCtx == EGL_NO_CONTEXT) {
            return -1;
        }
        if (!eglMakeCurrent(eglDisp, eglSurface, eglSurface, eglCtx)) {
            LOGE("MakeCurrent failed");
            return -1;
        }
        LOGI("egl initialize success!");

        int values[1] = {0};
        eglQueryContext(eglDisp, eglCtx, EGL_CONTEXT_CLIENT_VERSION, values);
        LOGI("EGLContext created, client version %d", values[0]);
        return 0;
    }

private:
    EGLConfig eglConf;
    EGLSurface eglSurface;
    EGLContext eglCtx;
    EGLDisplay eglDisp;
};

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

class AndroidAhardwareBuffer {
public:
    static std::unique_ptr<char[]> readAhardwareBuffer(AHardwareBuffer *buf, int &stride, int32_t fence = -1) {
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

    static AHardwareBuffer *allocAHardwareBuffer(uint32_t w, uint32_t h) {
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

    AndroidAhardwareBuffer(int width, int height) {
        hardwareBuffer = allocAHardwareBuffer(width, height);

        // 2. unity create texture
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
    }
    GLuint GetID() { return unity_tex; }
    ~AndroidAhardwareBuffer() {
        glDeleteTextures(1, &unity_tex);
        AHardwareBuffer_release(hardwareBuffer);
    }

private:
    GLuint unity_tex;
    AHardwareBuffer *hardwareBuffer;
};
