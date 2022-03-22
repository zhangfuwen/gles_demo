//
// Created by zhangfuwen on 2022/3/22.
//
#include "AndroidOffscreen.h"
#include "GLES.h"

#define VIEW_PORT_WIDTH 1921
#define VIEW_PORT_HEIGHT 1081
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

    if(auto ret = gles.Draw(); ret < 0) {
        gles.Finish();
        offscreen.eglFinish();
        return -1;
    }

    int res = 0;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    if(auto ret = GLES::readPixels("/data/1.png", 0, GL_BACK, width, height); ret < 0) {
        LOGE("failed to readPixels");
        res = ret;
    }

    gles.Finish();
    offscreen.eglFinish();

    return res;
}

