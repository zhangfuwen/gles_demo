//
// Created by zhangfuwen on 2022/3/22.
//
#include "Android.h"
#include "EGL.h"
#include "GLES.h"
#include "arm_counters.h"
#include "common.h"

#define VIEW_PORT_WIDTH 3712
#define VIEW_PORT_HEIGHT 3712
int main() {
    int width = VIEW_PORT_WIDTH;
    int height = VIEW_PORT_HEIGHT;
    EGL offscreen{};
    if (auto x = offscreen.Init(width, height); x < 0) {
        return -1;
    }
    defer de([&offscreen]() {
        offscreen.Finish();

    });

    GLES gles{};
    if (auto ret = gles.Init(); ret < 0) {
        offscreen.Finish();
        return -1;
    }
    defer dg([&gles]() {
        gles.Finish();
    });

    auto fboTex = GLES::CreateFBO(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    if (!fboTex) {
        return -1;
    }
    defer dfbo([fboTex]() {
        glDeleteFramebuffers(1, &fboTex.value());
    });

    auto buffer = new AndroidAHardwareBuffer(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    auto fboAndroid = GLES::CreateFBO(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT, false, buffer->GetID());
    if(!fboAndroid) {
        return -1;
    }
    defer dfbo1([fboAndroid]() {
      glDeleteFramebuffers(1, &fboAndroid.value());
    });

    auto render = [&gles, &offscreen](int num_frames = 1000) {
        glFinish();
        uint64_t nsec_acum = 0;
        uint64_t usec_acum_total = 0;
        for (int i = 1; i <= num_frames; i++) {
            timespec t1{}, t2{};
            clock_gettime(CLOCK_MONOTONIC, &t1);

            if (auto ret = gles.Draw(); ret < 0) {
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
        //        if(desc.name.find("VBIF") != std::string::npos
        //           || desc.category.find("VBIF") != std::string::npos
        //
        //                ||desc.name.find("GBIF") != std::string::npos
        //            || desc.category.find("GBIF") != std::string::npos
        //
        //               ||desc.name.find("UCHE") != std::string::npos
        //               || desc.category.find("UCHE") != std::string::npos
        //
        //               || desc.name.find("AXI") != std::string::npos
        //                  || desc.name.find("AXI") != std::string::npos
        //                     || desc.name.find("TP") != std::string::npos
        //                     || desc.name.find("TP") != std::string::npos
        //                        || desc.name.find("SP") != std::string::npos
        //                        || desc.name.find("SP") != std::string::npos
        //            )  {
        LOGI("desc name:%s, cat:%s, desc:%s", desc.name.c_str(), desc.category.c_str(), desc.description.c_str());
        counters.EnableCounter(c);
        //        }
    }

    LOGI("start");
    counters.BeginPass();
    LOGI("start");

    counters.BeginSample(0);
    LOGI("start");
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    LOGI("on offscreen");
    render(5000);
    counters.EndSample();

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

    if (auto ret = GLES::ReadPixels("/data/fbo.png", fboAndroid.value(), GL_COLOR_ATTACHMENT0, width, height); ret < 0) {
        LOGE("failed to ReadPixels");
        res = ret;
    }

    if (auto ret = GLES::ReadPixels("/data/fboTex.png", fboTex.value(), GL_COLOR_ATTACHMENT0, width, height); ret < 0) {
        LOGE("failed to ReadPixels");
        res = ret;
    }

    return res;
}
