//
// Created by zhangfuwen on 2022/3/22.
//
#include "../common/EGL.h"
#include "../common/GLES.h"
#include "../common/common.h"
#include "Android.h"
#include "arm_counters.h"
#include <android/log.h>
#include <fstream>

#if NATIVE_APP

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
    GLES::PrintTextureInfo(fboTex.value());
    defer dfbo([fboTex]() { glDeleteFramebuffers(1, &fboTex.value()); });

    auto buffer = new AndroidAHardwareBuffer(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    GLES::PrintTextureInfo(buffer->GetID());
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

            //******* glCear *****//
//            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
//            glClear(GL_COLOR_BUFFER_BIT);
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

    QCOMCounters counters;
    counters.Init();
    auto cs = counters.GetPublicCounterIds();
    for (auto &c : cs) {
        auto desc = counters.GetCounterDescription(c);
#if 1
        std::vector<std::string> criteria = {
            "M1_STALL", "M0_STALL"
//            "LRZ_LRZ","LRZ_TOTAL_PIXEL", "LRZ_VISIBLE_PIXEL"
//            "SP",
//            "VBIF", "GBIF", "UCHE", "AXI", "TP", "SP"
//            "LRZ_LRZ",
//            "LRZ_STALL_CYCLES_UCHE"
//            "LRZ_STALL_CYCLES_RB"
//            "LRZ_STALL_CYCLES_VC"
        };
        if(string_contains_any(desc.name, criteria)
            || string_contains_any(desc.category, criteria)
            )
        {
            LOGI("desc name:%s, cat:%s, desc:%s", desc.name.c_str(), desc.category.c_str(), desc.description.c_str());
            counters.EnableCounter(c);

        }
#else
        counters.EnableCounter(c);
#endif
    }

//    counters.BeginPass();

    //    counters.BeginSample(0);
    //    LOGI("start");
    //    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    //    LOGI("on offscreen");
    //    render(5000);
    //    counters.EndSample();

//    counters.BeginSample(1);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    LOG_BANNER("offscreen");
    render(500);
//    counters.EndSample();

//    counters.BeginSample(2);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboAndroid.value());
    LOG_BANNER("on ahardwarebuffer");
    render(500);
//    counters.EndSample();

//    counters.BeginSample(3);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboTex.value());
    LOG_BANNER("on texture");
    render(500);
//    counters.EndSample();

#if 0
    std::ofstream f1;
    f1.open("/data/diff.csv");
    for(int i = 1; i<=3; i++) {
        auto res = counters.GetCounterResult(i);
        if(i==1) {
            for(auto c : res) {
                f1 << counters.GetCounterDescription(c.counter).name << ", ";
            }
            f1 << "\n";
        }
        for(auto c : res) {
            f1 << hr(c.value.u32) << ", ";
        }
        f1 << "\n";

    }
    f1.close();
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
