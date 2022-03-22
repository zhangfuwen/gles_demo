project(test)

cmake_minimum_required(VERSION 3.1)

include(ExternalProject)
ExternalProject_Add(handycpp
        GIT_REPOSITORY https://github.com/zhangfuwen/handycpp
        GIT_TAG main
        SOURCE_DIR "${CMAKE_BINARY_DIR}/handycpp/src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/handycpp/build"
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
        TEST_COMMAND ""
        )

ExternalProject_add(happly
        GIT_REPOSITORY https://github.com/nmwsharp/happly
        GIT_TAG master
        SOURCE_DIR "${CMAKE_BINARY_DIR}/happly/src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/happly/build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
        )
if (CMAKE_SYSTEM_NAME MATCHES Android)
    add_executable(android_offscreen_test AndroidMain.cpp AndroidOffscreen.h GLES.cpp)
    target_compile_options(android_offscreen_test PRIVATE -std=c++17)
    target_link_libraries(android_offscreen_test EGL GLESv2 GLESv3 android jnigraphics nativewindow log)
    target_compile_features(android_offscreen_test INTERFACE cxx_std_17)

    add_dependencies(android_offscreen_test handycpp)
    add_dependencies(android_offscreen_test happly)
    target_include_directories(android_offscreen_test PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
    target_include_directories(android_offscreen_test PRIVATE ${CMAKE_BINARY_DIR}/happly/src)
endif ()


message(WARNING "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    add_executable(glfw_window glfw_window.cpp GLES.cpp)
    add_dependencies(glfw_window handycpp)
    add_dependencies(glfw_window happly)
    target_include_directories(glfw_window PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
    target_include_directories(glfw_window PRIVATE ${CMAKE_BINARY_DIR}/happly/src)
    target_link_libraries(glfw_window PRIVATE
            ${OPENGL_gl_LIBRARY}
            ${OPENGL_egl_LIBRARY}
            glfw
            )

    add_executable(glfw_offscreen glfw_offscreen.cpp GLES.cpp)
    add_dependencies(glfw_offscreen handycpp)
    add_dependencies(glfw_offscreen happly)
    target_include_directories(glfw_offscreen PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
    target_include_directories(glfw_offscreen PRIVATE ${CMAKE_BINARY_DIR}/happly/src)
    target_link_libraries(glfw_offscreen PRIVATE
            ${OPENGL_gl_LIBRARY}
            ${OPENGL_egl_LIBRARY}
            glfw
            )

endif ()


