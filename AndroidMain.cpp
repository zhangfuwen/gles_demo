//
// Created by zhangfuwen on 2022/3/22.
//
#include "AndroidOffscreen.h"
#include "GLES.h"

#define VIEW_PORT_WIDTH 3750
#define VIEW_PORT_HEIGHT 1750
int main() {
    int width = VIEW_PORT_WIDTH;
    int height = VIEW_PORT_HEIGHT;
    AndroidOffscreen offscreen;
    if(auto x = offscreen.eglInit(width, height); x < 0) {
        return -1;
    }

    GLES gles;
    if(auto ret = gles.Init(); ret < 0) {
        offscreen.eglFinish();
        return -1;
    }

    GLuint fboTex = GLES::GLES_CreateFBO(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);


    GLuint fbo;
    auto buffer = new AndroidAhardwareBuffer(VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer->GetID(), 0);

    if(auto ret =glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER); ret != GL_FRAMEBUFFER_COMPLETE) {
        FUN_ERROR("framebuffer incomplete %x", ret);
        return -1;
    }

    auto render = [&gles, &offscreen](int num_frames = 1000){
        glFinish();
      uint64_t nsec_acum = 0;
      uint64_t usec_acum = 0;
      for(int i = 0; i < num_frames; i++) {
            timespec t1, t2;
            clock_gettime(CLOCK_MONOTONIC, &t1);

            if(auto ret = gles.Draw(); ret < 0) {
                gles.Finish();
                offscreen.eglFinish();
                return -1;
            }
            glFinish();
            clock_gettime(CLOCK_MONOTONIC, &t2);
            uint64_t diff_nsec = (t2.tv_sec - t1.tv_sec)*1000000000 + t2.tv_nsec - t1.tv_nsec;
            nsec_acum += diff_nsec;
            if(i%100 == 1) {
                LOGI("%d round: time: %llu us", i/100, nsec_acum/1000);
                usec_acum += nsec_acum / 1000;
                nsec_acum=0;
            }
        }
        LOGI("total us: %llu", usec_acum);
        return 0;
    };

//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//    LOGI("on offscreen");
//    render(1000000000);

//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
//    LOGI("on ahardwarebuffer");
//    render(1000000000);
//
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboTex);
    LOGI("on texture");
    render(1000000000);




    int res = 0;

    if(auto ret = GLES::readPixels("/data/fbo.png", fbo, GL_COLOR_ATTACHMENT0, width, height); ret < 0) {
        LOGE("failed to readPixels");
        res = ret;
    }

    if(auto ret = GLES::readPixels("/data/fboTex.png", fboTex, GL_COLOR_ATTACHMENT0, width, height); ret < 0) {
        LOGE("failed to readPixels");
        res = ret;
    }

    gles.Finish();
    offscreen.eglFinish();

    return res;
}

