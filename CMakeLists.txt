project(test)

cmake_minimum_required(VERSION 3.1)


include(ExternalProject)
ExternalProject_Add(handycpp
        GIT_REPOSITORY https://github.com/zhangfuwen/handycpp
        GIT_TAG main
        SOURCE_DIR "${CMAKE_BINARY_DIR}/handycpp/src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/handycpp/build"
        CONFIGURE_COMMAND "ls"
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
        )
add_executable(test test.cpp)
target_link_libraries(test EGL GLESv2 GLESv3 android jnigraphics nativewindow log)
target_compile_features(test INTERFACE cxx_std_20)

add_dependencies(test handycpp)
target_include_directories(test PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
