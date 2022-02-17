#!/bin/bash

cmake -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake  \
  -DANDROID_ABI=armeabi-v7a \
  -DANDROID_ARM_NEON=ON \
  -DANDROID_PLATFORM=android-29 \
    -S . -B build
cmake --build build


