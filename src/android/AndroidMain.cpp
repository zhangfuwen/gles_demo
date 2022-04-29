//
// Created by zhangfuwen on 2022/3/22.
//
#include "../common/EGL.h"
#include "../common/GLES.h"
#include "../common/common.h"
#include "Android.h"
#include "arm_counters.h"
#include <android/log.h>

#if NATIVE_APP
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

#endif

#define VIEW_PORT_WIDTH 3712
#define VIEW_PORT_HEIGHT 3712

bool string_contains_any(std::string s, std::vector<std::string> substrs) {
    return std::any_of(substrs.begin(), substrs.end(), [&s](const std::string &substr) {
        return s.find(substr) != std::string::npos;
    });
}

#if not NATIVE_APP
int main() {
    int width = VIEW_PORT_WIDTH;
    int height = VIEW_PORT_HEIGHT;
    EGL offscreen{};
    if (auto x = offscreen.Init(width, height); x < 0) {
        return -1;
    }
    defer de([&offscreen]() { offscreen.Finish(); });

    GLES gles{};
    if (auto ret = gles.Init(); ret < 0) {
        offscreen.Finish();
        return -1;
    }
    defer dg([&gles]() { gles.Finish(); });

    auto fboTex = GLES::CreateFBO(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    if (!fboTex) {
        return -1;
    }
    defer dfbo([fboTex]() { glDeleteFramebuffers(1, &fboTex.value()); });

    auto buffer = new AndroidAHardwareBuffer(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    auto fboAndroid = GLES::CreateFBO(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT, false, buffer->GetID());
    if (!fboAndroid) {
        return -1;
    }
    defer dfbo1([fboAndroid]() { glDeleteFramebuffers(1, &fboAndroid.value()); });

    auto render = [&gles, &offscreen](int num_frames = 1000) {
        glFinish();
        uint64_t nsec_acum = 0;
        uint64_t usec_acum_total = 0;
        for (int i = 1; i <= num_frames; i++) {
            timespec t1{}, t2{};
            clock_gettime(CLOCK_MONOTONIC, &t1);

            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            if (auto ret = gles.Draw(); ret < 0) {
                LOGE("draw failed");
                gles.Finish();
                offscreen.Finish();
                return -1;
            }
            glFinish();
            clock_gettime(CLOCK_MONOTONIC, &t2);
            uint64_t diff_nsec = (t2.tv_sec - t1.tv_sec) * 1000000000 + t2.tv_nsec - t1.tv_nsec;
            nsec_acum += diff_nsec;
            if (i % 100 == 0) {
                LOGI("%d round: time: %lu us", i / 100, nsec_acum / 1000);
                usec_acum_total += nsec_acum / 1000;
                nsec_acum = 0;
            }
        }
        LOGI("total us: %lu", usec_acum_total);
        return 0;
    };

    QCOMCounters counters;
    counters.Init();
    auto cs = counters.GetPublicCounterIds();
    for (auto &c : cs) {
        auto desc = counters.GetCounterDescription(c);
#if 0
        std::vector<std::string> criteria = {
            "VBIF", "GBIF", "UCHE", "AXI", "TP", "SP"
        };
        if(string_contains_any(desc.name, criteria)
            || string_contains_any(desc.category, criteria)
            )
        {
            LOGI("desc name:%s, cat:%s, desc:%s", desc.name.c_str(), desc.category.c_str(), desc.description.c_str());
            counters.EnableCounter(c);

        }
#else
        LOGI("desc name:%s, cat:%s, desc:%s", desc.name.c_str(), desc.category.c_str(), desc.description.c_str());
        counters.EnableCounter(c);
#endif
    }

    LOGI("start");
    counters.BeginPass();
    LOGI("start");

    //    counters.BeginSample(0);
    //    LOGI("start");
    //    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    //    LOGI("on offscreen");
    //    render(5000);
    //    counters.EndSample();

    counters.BeginSample(1);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    LOGI("on offscreen");
    render(5000);
    counters.EndSample();

    counters.BeginSample(2);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboAndroid.value());
    LOGI("on ahardwarebuffer");
    render(5000);
    counters.EndSample();

    counters.BeginSample(3);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboTex.value());
    LOGI("on texture");
    render(5000);
    counters.EndSample();

#if 0
    for(int i = 0; i<=3; i++) {
        auto res = counters.GetCounterData({(unsigned)i} );
        if(i==0) {
            for(auto c : res) {
                LOGI("%s,", counters.GetCounterDescription(c.counter).name.c_str());
            }
        }
        for(auto c : res) {
            LOGI("%d,", c.value.u32);
        }

    }
#endif

    int res = 0;

    if (auto ret = GLES::ReadPixels("/data/fbo0.png", 0, GL_COLOR_ATTACHMENT0, width, height);
        ret < 0) {
        LOGE("failed to ReadPixels");
        res = ret;
    }

    if (auto ret = GLES::ReadPixels("/data/fbo.png", fboAndroid.value(), GL_COLOR_ATTACHMENT0, width, height);
        ret < 0) {
        LOGE("failed to ReadPixels");
        res = ret;
    }

    if (auto ret = GLES::ReadPixels("/data/fboTex.png", fboTex.value(), GL_COLOR_ATTACHMENT0, width, height); ret < 0) {
        LOGE("failed to ReadPixels");
        res = ret;
    }

    return res;
}

#endif
