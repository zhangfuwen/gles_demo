//
// Created by zhangfuwen on 2022/4/28.
//
#include "EGL.h"
#include "common.h"

int EGL::Init(int width, int height, EGLNativeWindowType win /* = nullptr */) {
    // clang-format off
    const EGLint confAttr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, // very important!
        EGL_SURFACE_TYPE, win? EGL_WINDOW_BIT : EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE};

    const EGLint ctxAttr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3, // very important!
        EGL_NONE};

    const EGLint surfaceAttr[] = {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_NONE};
    // clang-format on

    EGLint eglMajVers, eglMinVers;
    EGLint numConfigs;

    if (eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY); eglDisp == EGL_NO_DISPLAY) {
        LOGE("Unable to open connection to local windowing system");
        return -1;
    }
    if (!eglInitialize(eglDisp, &eglMajVers, &eglMinVers)) {
        LOGE("Unable to initialize EGL");
        return -1;
    }

    LOGI("EGL Init with version %d.%d", eglMajVers, eglMinVers);
    if (!eglChooseConfig(eglDisp, confAttr, &eglConf, 1, &numConfigs)) {
        LOGE("some config is wrong");
        return -1;
    } else {
        LOGI("all configs are OK");
    }

    // create PbufferSurface
    if(win) {
        eglSurface = eglCreateWindowSurface(eglDisp, eglConf, win, nullptr);
        if (eglSurface == EGL_NO_SURFACE) {
            switch (eglGetError()) {
                case EGL_BAD_ALLOC:
                    LOGE("Not enough resources available");
                    break;
                case EGL_BAD_CONFIG:
                    LOGE("provided EGLConfig is invalid");
                    break;
                case EGL_BAD_PARAMETER:
                    LOGE("provided EGL_WIDTH and EGL_HEIGHT is invalid");
                    break;
                case EGL_BAD_MATCH:
                    LOGE("Check window and EGLConfig attributes");
                    break;
                default:
                    LOGE("Unknown error");
                    break;
            }
            EGL_CHECK_ERROR_RET(-1, "eglCreateWindowSurface");
        }

    } else {
        eglSurface = eglCreatePbufferSurface(eglDisp, eglConf, surfaceAttr);
        if (eglSurface == EGL_NO_SURFACE) {
            switch (eglGetError()) {
                case EGL_BAD_ALLOC:
                    LOGE("Not enough resources available");
                    break;
                case EGL_BAD_CONFIG:
                    LOGE("provided EGLConfig is invalid");
                    break;
                case EGL_BAD_PARAMETER:
                    LOGE("provided EGL_WIDTH and EGL_HEIGHT is invalid");
                    break;
                case EGL_BAD_MATCH:
                    LOGE("Check window and EGLConfig attributes");
                    break;
                default:
                    LOGE("Unknown error");
                    break;
            }
            EGL_CHECK_ERROR_RET(-1, "eglCreatePbufferSurface");
        }

    }

    if (eglCtx = eglCreateContext(eglDisp, eglConf, EGL_NO_CONTEXT, ctxAttr); eglCtx == EGL_NO_CONTEXT) {
        EGL_CHECK_ERROR_RET(-1, "eglCreateContext");
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