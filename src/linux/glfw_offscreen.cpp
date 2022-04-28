//
// Created by zhangfuwen on 2022/3/22.
//


#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLES3/gl32.h>
#if USE_NATIVE_OSMESA
#define GLFW_EXPOSE_NATIVE_OSMESA
 #include <GLFW/glfw3native.h>
#endif


#include <stdlib.h>
#include <stdio.h>

#include "handycpp/image.h"
#include "../common/GLES.h"

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

int main(void)
{
    GLFWwindow* window;
    int width = 640, height = 480;
    bool GLES_OR_OPENGL = true; // true GLES

    glfwSetErrorCallback(error_callback);

//    glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_FALSE);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    if(GLES_OR_OPENGL) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    } else {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }


    glfwMakeContextCurrent(window);

    FUN_INFO("VENDOR: %s", glGetString(GL_VENDOR));
    FUN_INFO("RENDER: %s", glGetString(GL_RENDERER));
    FUN_INFO("VERSION: %s", glGetString(GL_VERSION));

    GLES gles;
    if(auto ret = gles.Init(); ret < 0) {
        return ret;
    }

    // NOTE: OpenGL error checks have been omitted for brevity
    glfwGetFramebufferSize(window, &width, &height);
    FUN_INFO("width:%d, height:%d", width, height);

    glViewport(0, 0, width, height);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    if(auto ret = gles.Draw(); ret < 0) {
        return ret;
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    if(auto ret = GLES::ReadPixels("1.png", 0, GL_BACK, width, height); ret < 0) {
        return ret;
    }

    gles.Finish();

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}