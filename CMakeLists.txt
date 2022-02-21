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
        TEST_COMMAND ""
        )

ExternalProject_add(happly
        GIT_REPOSITORY  https://github.com/nmwsharp/happly
        GIT_TAG master
        SOURCE_DIR "${CMAKE_BINARY_DIR}/happly/src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/happly/build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
        )
add_executable(test test.cpp)
target_link_libraries(test EGL GLESv2 GLESv3 android jnigraphics nativewindow log)
target_compile_features(test INTERFACE cxx_std_20)

add_dependencies(test handycpp)
add_dependencies(test happly)
target_include_directories(test PRIVATE ${CMAKE_BINARY_DIR}/handycpp/src/include)
target_include_directories(test PRIVATE ${CMAKE_BINARY_DIR}/happly/src)
