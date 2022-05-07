//
// Created by zhangfuwen on 2022/5/7.
//

#include "../common/EGL.h"
#include "../common/GLES.h"
#include "../common/common.h"
#include "Android.h"
#include "arm_counters.h"
#include <android/log.h>
#include <fstream>

#include <android/native_activity.h>
#include <android_native_app_glue.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native_app", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "naitve_app", __VA_ARGS__))
extern "C" {

class App {
public:
    void onAppCmd(android_app *state, int32_t cmd) {
        LOGI("cmd %d", cmd);
        switch (cmd) {
            case APP_CMD_INIT_WINDOW: {
                LOGI("cmd %d, INIT_WINDOW", cmd);

                if(state->window != nullptr) {
                    auto w = ANativeWindow_getWidth(state->window);
                    auto h = ANativeWindow_getHeight(state->window);
                    LOGI("width:%d, height:%d", w, h);
                    m_egl.Init(w, h, state->window);
                    m_gles.Init();
                }

            } break;
            case APP_CMD_TERM_WINDOW:
                if(m_egl.GetContext() != EGL_NO_CONTEXT) {
                    m_gles.Finish();
                    m_egl.Finish();
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
        if(m_egl.GetContext() != EGL_NO_CONTEXT) {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            m_gles.Draw();
            eglSwapBuffers(eglGetCurrentDisplay(), m_egl.GetSurface());
        } else {
            //            LOGI("no context");
        }
    }

private:
    EGL m_egl{};
    GLES m_gles{};
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
        LOGI("cmd %d", cmd);
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
        auto pApp = (App*)state->userData;
        pApp->Draw();
    }
}
} // end extern "C"
