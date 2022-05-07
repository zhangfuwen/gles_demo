//
// Created by zhangfuwen on 2022/5/7.
//

#include "../common/EGL.h"
#include "../common/GLES.h"
#include "../common/Vulkan.h"
#include "../common/common.h"
#include "Android.h"
#include "arm_counters.h"
#include <android/log.h>
#include <fstream>

#include <android/native_activity.h>
#include <android_native_app_glue.h>
extern "C" {

class App {
public:
    void onAppCmd(android_app *state, int32_t cmd) {
        LOGD("cmd %d", cmd);
        switch (cmd) {
            case APP_CMD_INIT_WINDOW: {
                LOGD("cmd %d, INIT_WINDOW", cmd);

                if (state->window != nullptr) {
                    w = ANativeWindow_getWidth(state->window);
                    h = ANativeWindow_getHeight(state->window);
                    LOGI("width:%d, height:%d", w, h);
                    m_egl.Init(w, h, state->window);
                    m_gles.Init();
                    if (auto ret = m_vulkan.Init(); ret < 0) {
                        LOGI("failed to init vulkan");
                    }
                    int size;
                    m_fd = m_vulkan.CreateMemObjFd(w, h, &size);
                    LOGI("m_fd is %d", m_fd);
                    auto tex = GLES::CreateTextureFromFd(w, h, size, m_fd);
                    {
                        glBindTexture(GL_TEXTURE_2D, tex.value());
                        GLint ret;
                        glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_IMMUTABLE_FORMAT, &ret);
                        if(ret == GL_FALSE) {
                            LOGI("texture is mutable");
                        } else {
                            LOGI("texture is immutable");
                        }

                        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, &ret);
                        if(ret == GL_OPTIMAL_TILING_EXT) {
                            LOGI("tiling optimal");
                        } else if(ret == GL_LINEAR_TILING_EXT) {
                            LOGI("tiling linear");
                        } else {
                            LOGI("tiling unknown %x", ret);
                        }

                        GLint width = 0;
                        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
                        if (width > 0) {
                            LOGI("texture has storeage");
                        } else {
                            LOGI("texture has no storage");
                        }



                    }
                    if (auto ret = GLES::CreateFBO(w, h, false, tex.value()); ret.has_value()) {
                        m_fbo = ret.value();
                    } else {
                        LOGE("m_fd:%d, tex:%d, w:%d, h:%d, size:%d", m_fd, tex, w, h, size);
                        exit(-1);
                    }
                }

            } break;
            case APP_CMD_TERM_WINDOW:
                if (m_egl.GetContext() != EGL_NO_CONTEXT) {
                    m_gles.Finish();
                    m_egl.Finish();
                    m_vulkan.Finish();
                }
                break;
            case APP_CMD_SAVE_STATE:
                state->savedState = this;
                state->savedStateSize = sizeof(App);
            default:
                break;
        }
    }
    void Draw() {
        if (m_egl.GetContext() != EGL_NO_CONTEXT) {

//            measure_time([&]() {
//                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//                m_gles.Draw();
//                glFinish();
//            }).print_time_ms("onscreen");

            measure_time([&]() {
              glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
              glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
              glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
              m_gles.Draw();
              glFinish();
            }).print_time_ms(string_format("fbo %d", m_fbo));

            m_vulkan.Draw();
            eglSwapBuffers(eglGetCurrentDisplay(), m_egl.GetSurface());
        } else {
            //            LOGI("no context");
        }
    }

private:
    int w, h;
    EGL m_egl{};
    GLES m_gles{};
    Vulkan m_vulkan{};
    int m_fd;
    GLuint m_fbo = 0;
};
void android_main_override(struct android_app *state) {
    if (state == nullptr) {
        LOGI("state is nullptr");
        return;
    }
    LOGI("android_app %p", state);
    App app;
    state->userData = &app;

    state->onAppCmd = [](android_app *state, int32_t cmd) {
        LOGD("cmd %d", cmd);
        auto *app = (App *)state->userData;
        app->onAppCmd(state, cmd);
    };

    LOGI("starting loop");
    while (!state->destroyRequested) {
        int ident;
        int events;
        struct android_poll_source *source = nullptr;
        while ((ident = ALooper_pollAll(0, nullptr, &events, (void **)&source)) >= 0) {
            if (source != nullptr) {
                source->process(state, source);
            }

            if (ident == LOOPER_ID_USER) {
            }
        }
        auto pApp = (App *)state->userData;
        pApp->Draw();
    }
}
} // end extern "C"
