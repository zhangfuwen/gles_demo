project(test)

cmake_minimum_required(VERSION 3.1)

include(ExternalProject)


message(WARNING "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")

### android ###
if (CMAKE_SYSTEM_NAME MATCHES Android)

    add_library(native_app_glue STATIC
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
    target_include_directories(native_app_glue PUBLIC
            ${ANDROID_NDK}/sources/android/native_app_glue)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

    add_library(native_app SHARED
            src/common/EGL.cpp
            src/common/GLES.cpp
            src/common/Vulkan.cpp
            src/android/Android.cpp
            src/android/NativeApp.cpp
            src/android/arm_counters.cpp)
    target_include_directories(native_app PUBLIC
            ${ANDROID_NDK}/sources/android/native_app_glue)
    target_compile_options(native_app PRIVATE -std=c++17 -DNATIVE_APP=1)
    target_link_libraries(native_app
            EGL
            GLESv2
            GLESv3
            android
            vulkan
            jnigraphics
            nativewindow
            log)
    target_compile_features(native_app INTERFACE cxx_std_17)

    # offscreen
    add_executable(android_offscreen_test
            src/common/EGL.cpp
            src/common/GLES.cpp
            src/android/Android.cpp
            src/common/Vulkan.cpp
            src/android/AndroidMain.cpp
            src/android/arm_counters.cpp)
    target_compile_options(android_offscreen_test PRIVATE -std=c++17)
    target_link_libraries(android_offscreen_test
            EGL
            GLESv2
            GLESv3
            vulkan
            android
            jnigraphics
            native_app_glue
            nativewindow
            log)
    target_compile_features(android_offscreen_test INTERFACE cxx_std_17)

    # vulkan
    add_executable(vulkan_test
            src/common/Vulkan.cpp
            )
    target_compile_options(vulkan_test PRIVATE -std=c++17)
    target_link_libraries(vulkan_test EGL vulkan GLESv2 GLESv3 android jnigraphics nativewindow log)
    target_compile_features(vulkan_test INTERFACE cxx_std_17)


    # gpu top
    add_executable(gputop src/tools/AndroidGpuTop.cpp)
    target_compile_options(gputop PRIVATE -std=c++17)
    target_compile_features(gputop INTERFACE cxx_std_17)

    # android tools
    add_executable(read_pipe src/tools/read_pipe.cpp)
    add_executable(mem src/tools/mem.cpp)
    target_compile_options(mem PRIVATE -std=c++17)
endif ()

### linux ###
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)

    # glfw window
    add_executable(glfw_window src/linux/glfw_window.cpp src/common/GLES.cpp)
    target_link_libraries(glfw_window PRIVATE
            ${OPENGL_gl_LIBRARY}
            ${OPENGL_egl_LIBRARY}
            glfw
            )

    # glfw offscreen
    add_executable(glfw_offscreen src/linux/glfw_offscreen.cpp src/common/GLES.cpp)
    target_link_libraries(glfw_offscreen PRIVATE
            ${OPENGL_gl_LIBRARY}
            ${OPENGL_egl_LIBRARY}
            glfw
            )

    # gputop
    add_executable(gputop src/tools/AndroidGpuTop.cpp)
    target_compile_options(gputop PRIVATE -std=c++17)
    target_compile_features(gputop INTERFACE cxx_std_17)

endif (CMAKE_SYSTEM_NAME MATCHES Linux)



if (VCPKG_TOOLCHAIN MATCHES ON)
    find_package(handycpp CONFIG REQUIRED)
    find_path(HAPPLY_INCLUDE_DIRS "happly.h")

    if (CMAKE_SYSTEM_NAME MATCHES Android)
        target_link_libraries(gputop PRIVATE handycpp::handycpp)
        target_include_directories(gputop PRIVATE ${HAPPLY_INCLUDE_DIRS})

        target_link_libraries(android_offscreen_test PRIVATE handycpp::handycpp)
        target_include_directories(android_offscreen_test PRIVATE ${HAPPLY_INCLUDE_DIRS})
    endif ()

    if (CMAKE_SYSTEM_NAME MATCHES Linux)
        target_link_libraries(glfw_window PRIVATE handycpp::handycpp)
        target_include_directories(glfw_window PRIVATE ${HAPPLY_INCLUDE_DIRS})

        target_link_libraries(glfw_offscreen PRIVATE handycpp::handycpp)
        target_include_directories(glfw_offscreen PRIVATE ${HAPPLY_INCLUDE_DIRS})

        target_link_libraries(gputop PRIVATE handycpp::handycpp)
        target_include_directories(gputop PRIVATE ${HAPPLY_INCLUDE_DIRS})
    endif ()
else ()
    message("not having vcpkg")
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
        add_dependencies(native_app handycpp)
        add_dependencies(native_app happly)
        target_include_directories(native_app PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
        target_include_directories(native_app PRIVATE ${CMAKE_BINARY_DIR}/happly/src)

        add_dependencies(android_offscreen_test handycpp)
        add_dependencies(android_offscreen_test happly)
        target_include_directories(android_offscreen_test PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
        target_include_directories(android_offscreen_test PRIVATE ${CMAKE_BINARY_DIR}/happly/src)

        add_dependencies(gputop handycpp)
        add_dependencies(gputop happly)
        target_include_directories(gputop PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
        target_include_directories(gputop PRIVATE ${CMAKE_BINARY_DIR}/happly/src)
    endif ()

    if (CMAKE_SYSTEM_NAME MATCHES Linux)
        add_dependencies(glfw_window handycpp)
        add_dependencies(glfw_window happly)
        target_include_directories(glfw_window PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
        target_include_directories(glfw_window PRIVATE ${CMAKE_BINARY_DIR}/happly/src)

        add_dependencies(glfw_offscreen handycpp)
        add_dependencies(glfw_offscreen happly)
        target_include_directories(glfw_offscreen PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
        target_include_directories(glfw_offscreen PRIVATE ${CMAKE_BINARY_DIR}/happly/src)

        add_dependencies(gputop handycpp)
        add_dependencies(gputop happly)
        target_include_directories(gputop PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
        target_include_directories(gputop PRIVATE ${CMAKE_BINARY_DIR}/happly/src)
    endif ()

endif ()

