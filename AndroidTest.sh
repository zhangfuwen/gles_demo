#!/bin/bash
BUILD_PATH=build-android
adb shell rm /data/1.png
adb shell rm /data/2.png
adb shell rm /data/3.png
adb shell rm /data/4.png
adb push ./data/bun_zipper.ply /data/
adb push ./${BUILD_PATH}/android_offscreen_test /data/
adb shell "cd /data && ./test"
adb pull /data/1.png
adb pull /data/2.png
adb pull /data/3.png
adb pull /data/4.png
#adb pull /data/2.bmp
