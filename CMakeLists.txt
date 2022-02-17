project(test)

cmake_minimum_required(VERSION 3.1)

add_executable(test test.cpp)
target_link_libraries(test EGL GLESv2 GLESv3 android jnigraphics nativewindow)