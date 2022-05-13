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
#include <thread>

#include <android/native_activity.h>
#include <android_native_app_glue.h>

#define glTexRedirect2D(tex1, tex2) do { \
        GLuint tex[2]; \
        tex[0] = tex1; \
        tex[1] = tex2; \
        glGenTextures(-1, tex);              \
    } while(0)

#define VIEW_PORT_WIDTH 3712
#define VIEW_PORT_HEIGHT 3712

int measure2(int w, int h, GLES &gles, EGL &offscreen) {
    int width = w;
    int height = h;

    // create GLES texture backed fbo for rendering
    GLuint unityTex;
    glGenTextures(1, &unityTex);
    glBindTexture(GL_TEXTURE_2D, unityTex);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_OPTIMAL_TILING_EXT);
    //        GL_CHECK_ERROR("");
    defer dt([]() { glBindTexture(GL_TEXTURE_2D, 0); });
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GL_CHECK_ERROR_RET({}, "glTexImage2D failed");



    // create vulkan memobj backed fbo for rendering
    GLuint fboMemObj;
    Vulkan m_vulkan{};
    int size = 0;
    if (auto ret = m_vulkan.Init(); ret < 0) {
        LOGI("failed to init vulkan");
        exit(-1);
    }
    int fd = m_vulkan.CreateMemObjFd(w, h, &size);
    auto texFromMemObj = GLES::CreateTextureFromFd(w, h, size, fd);

    /**
     * render function used for measurement
     * @param num_frames how many frames do you want to render?
     */
    auto render = [&gles, &offscreen](int num_frames = 1000) {
        glFinish();
        uint64_t nsec_acum = 0;
        uint64_t usec_acum_total = 0;
        for (int i = 1; i <= num_frames; i++) {
            timespec t1{}, t2{};
            clock_gettime(CLOCK_MONOTONIC, &t1);

            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            //            glHint(GL_BINNING_CONTROL_HINT_QCOM, GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM);
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

    auto fboOnTex = GLES::CreateFBO(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT, false, unityTex);
    if (!fboOnTex) {
        return -1;
    }
    defer dfbo([fboOnTex]() { glDeleteFramebuffers(1, &fboOnTex.value()); });

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboOnTex.value());
    render(500);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    // TODO: replace unityTex with vulkan memobj
    glTexRedirect2D(unityTex, texFromMemObj.value());
    GL_CHECK_ERROR("");

    render(500);

    {
        auto fbo = GLES::CreateFBO(w, h, false, unityTex);
        if(!fbo) {
            LOGE("failed to create fbo");
            GL_CHECK_ERROR("");
            exit(-1);
        }
        if (auto ret = GLES::ReadPixels("/data/fboTexUnity.png", fbo.value(), GL_COLOR_ATTACHMENT0, width, height); ret < 0) {
            LOGE("failed to ReadPixels");
        }

    }

    GLuint tempTex;
    glGenTextures(1, &tempTex);
    glBindTexture(GL_TEXTURE_2D, tempTex);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_OPTIMAL_TILING_EXT);
    //        GL_CHECK_ERROR("");
    defer dt1([]() { glBindTexture(GL_TEXTURE_2D, 0); });
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GL_CHECK_ERROR_RET({}, "glTexImage2D failed");

    // TODO: replace tempTex with vulkan memobj
    glTexRedirect2D(tempTex, texFromMemObj.value());
    GL_CHECK_ERROR("");


    // read texture
    {
        auto fbo = GLES::CreateFBO(w, h, false, tempTex);
        GL_CHECK_ERROR("");
        if(!fbo) {
            LOGE("failed to create fbo");
            GL_CHECK_ERROR("");
            exit(-1);
        }
        if (auto ret = GLES::ReadPixels("/data/fboTexTemp.png", fbo.value(), GL_COLOR_ATTACHMENT0, width, height); ret < 0) {
            GL_CHECK_ERROR("");
            LOGE("failed to ReadPixels");
        }

    }

    int res = 0;
    return res;
}

extern "C" {

class App {
public:
    void onAppCmd(android_app *state, int32_t cmd) {
        LOGD("cmd %d", cmd);
        switch (cmd) {
            case APP_CMD_INIT_WINDOW: {
                LOGD("cmd %d, INIT_WINDOW", cmd);

                if (state->window != nullptr) {
//                    std::thread t(measure2);
                    w = ANativeWindow_getWidth(state->window);
                    h = ANativeWindow_getHeight(state->window);
                    LOGI("width:%d, height:%d", w, h);
                    m_egl.Init(w, h, state->window);
                    m_gles.Init();
                    if (auto ret = m_vulkan.Init(); ret < 0) {
                        LOGI("failed to init vulkan");
                    }
                    int size = 0;
                    m_fd = m_vulkan.CreateMemObjFd(w, h, &size);
                    LOGI("m_fd is %d", m_fd);
                    auto tex = GLES::CreateTextureFromFd(w, h, size, m_fd);
                    GLES::PrintTextureInfo(tex.value());
                    if (auto ret = GLES::CreateFBO(w, h, false, tex.value()); ret.has_value()) {
                        m_fboMemObj = ret.value();
                    } else {
                        LOGE("m_fd:%d, tex:%d, w:%d, h:%d, size:%d", m_fd, tex, w, h, size);
                        exit(-1);
                    }
                    auto buffer = new AndroidAHardwareBuffer(w, h);
                    auto ret = GLES::CreateFBO(w, h, false, buffer->GetID());
                    if(ret.has_value()) {
                        m_fboAhardwareBuffer = ret.value();
                    } else {
                        LOGE("failed to create fbo");
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
            measure2(w, h, m_gles, m_egl);

            measure_time([&]() {
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                m_gles.Draw();
                glFinish();
            }).print_time_ms("onscreen");

            measure_time([&]() {
              glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fboMemObj);
              glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
              glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
              m_gles.Draw();
              glFinish();
            }).print_time_us(string_format("fboMemObj %d", m_fboMemObj));
            measure_time([&]() {
              glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fboAhardwareBuffer);
              glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
              glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
              m_gles.Draw();
              glFinish();
            }).print_time_us(string_format("fboAhardwarebuffer %d", m_fboAhardwareBuffer));

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
    GLuint m_fboMemObj = 0;
    GLuint m_fboAhardwareBuffer = 0;
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
