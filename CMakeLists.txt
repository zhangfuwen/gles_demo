project(test)

cmake_minimum_required(VERSION 3.1)

include(ExternalProject)



if (CMAKE_SYSTEM_NAME MATCHES Android)
    add_executable(android_offscreen_test AndroidMain.cpp AndroidOffscreen.h GLES.cpp)
    target_compile_options(android_offscreen_test PRIVATE -std=c++17)
    target_link_libraries(android_offscreen_test EGL GLESv2 GLESv3 android jnigraphics nativewindow log)
    target_compile_features(android_offscreen_test INTERFACE cxx_std_17)


    add_executable(gputop AndroidGpuTop.cpp)
    target_compile_options(gputop PRIVATE -std=c++17)
    #        target_link_libraries(gputop )
    target_compile_features(gputop INTERFACE cxx_std_17)

    add_executable(mem mem.cpp)
    target_compile_options(mem PRIVATE -std=c++17)

endif ()


message(WARNING "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    add_executable(glfw_window glfw_window.cpp GLES.cpp)
    target_link_libraries(glfw_window PRIVATE
            ${OPENGL_gl_LIBRARY}
            ${OPENGL_egl_LIBRARY}
            glfw
            )

    add_executable(glfw_offscreen glfw_offscreen.cpp GLES.cpp)
    target_link_libraries(glfw_offscreen PRIVATE
            ${OPENGL_gl_LIBRARY}
            ${OPENGL_egl_LIBRARY}
            glfw
            )

        add_executable(gputop AndroidGpuTop.cpp)

        target_compile_options(gputop PRIVATE -std=c++17)
        target_compile_features(gputop INTERFACE cxx_std_17)

endif ()


if(VCPKG_TOOLCHAIN MATCHES ON)
    find_package(handycpp CONFIG REQUIRED)
    find_path(HAPPLY_INCLUDE_DIRS "happly.h")

    if (CMAKE_SYSTEM_NAME MATCHES Android)
        target_link_libraries(gputop PRIVATE handycpp::handycpp)
        target_include_directories(gputop PRIVATE ${HAPPLY_INCLUDE_DIRS})

        target_link_libraries(android_offscreen_test PRIVATE handycpp::handycpp)
        target_include_directories(android_offscreen_test PRIVATE ${HAPPLY_INCLUDE_DIRS})
    endif()

    if (CMAKE_SYSTEM_NAME MATCHES Linux)
        target_link_libraries(glfw_window PRIVATE handycpp::handycpp)
        target_include_directories(glfw_window PRIVATE ${HAPPLY_INCLUDE_DIRS})

        target_link_libraries(glfw_offscreen PRIVATE handycpp::handycpp)
        target_include_directories(glfw_offscreen PRIVATE ${HAPPLY_INCLUDE_DIRS})

        target_link_libraries(gputop PRIVATE handycpp::handycpp)
        target_include_directories(gputop PRIVATE ${HAPPLY_INCLUDE_DIRS})
    endif()
else()
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
            GIT_REPOSITORY  https://github.com/nmwsharp/happly
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
        add_dependencies(android_offscreen_test handycpp)
        add_dependencies(android_offscreen_test happly)
        target_include_directories(android_offscreen_test PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
        target_include_directories(android_offscreen_test PRIVATE ${CMAKE_BINARY_DIR}/happly/src)

        add_dependencies(gputop handycpp)
        add_dependencies(gputop happly)
        target_include_directories(gputop PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
        target_include_directories(gputop PRIVATE ${CMAKE_BINARY_DIR}/happly/src)
    endif()

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
    endif()

endif()

add_executable(read_pipe read_pipe.cpp)
