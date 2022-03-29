//#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <GLES3/gl32.h>
#include <handycpp/image.h>
#include <sys/time.h>
#include <handycpp/time.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

#define VIEW_PORT_WIDTH 800
#define VIEW_PORT_HEIGHT 600

#include "GLES.h"
inline int64_t MeasureCyclesPerSecond(int timeout_ms) {
    int64_t t[2];
    t[0] = handycpp::time::cycle_clock::Now();
    handycpp::time::timer timer;
    timer.setTimeout([&t]() {
        t[1] = handycpp::time::cycle_clock::Now();
    }, timeout_ms);

    while(!timer.stopped()) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    }
    return (t[1] - t[0]) * 1000 / timeout_ms;
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();

    int64_t cyclesPerSec = MeasureCyclesPerSecond(1000);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    GLES gles;
    if(auto ret = gles.Init(); ret < 0 ) {
        return ret;
    }
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        struct timeval t[2];
        uint64_t c[2];
        gettimeofday(&t[0], nullptr);
        c[0] = handycpp::time::cycle_clock::Now();
        // input
        // -----
        processInput(window);
        FUN_INFO("VENDOR: %s", glGetString(GL_VENDOR));
        FUN_INFO("RENDER: %s", glGetString(GL_RENDERER));
        FUN_INFO("VERSION: %s", glGetString(GL_VERSION));
        // render
        // ------
        glViewport(0,0, VIEW_PORT_WIDTH, VIEW_PORT_HEIGHT);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if(auto ret = gles.Draw(); ret < 0) {
            return ret;
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        gettimeofday(&t[1], nullptr);
        c[1] = handycpp::time::cycle_clock::Now();

        FUN_INFO("clock %lu %lu", (c[1]-c[0]) *1000000/cyclesPerSec, ((long long)t[1].tv_sec - t[0].tv_sec)*1000000 + (t[1].tv_usec - t[0].tv_usec));


            struct timespec time1 = {0, 0};
            clock_gettime(CLOCK_REALTIME, &time1);
            printf("CLOCK_REALTIME: %d, %d\n", time1.tv_sec, time1.tv_nsec);
            clock_gettime(CLOCK_MONOTONIC, &time1);
            printf("CLOCK_MONOTONIC: %d, %d\n", time1.tv_sec, time1.tv_nsec);
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
            printf("CLOCK_PROCESS_CPUTIME_ID: %d, %d\n", time1.tv_sec, time1.tv_nsec);
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time1);
            printf("CLOCK_THREAD_CPUTIME_ID: %d, %d\n", time1.tv_sec, time1.tv_nsec);
            printf("\n%d\n", time(NULL));
//            sleep(1);
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    gles.Finish();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
