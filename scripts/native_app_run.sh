#!/bin/bash

function na_stop()
{
    adb shell am force-stop com.example.native_activity
}

function na_start()
{
  filePath=$1
    file=$(basename $1)
    dir=$(dirname $1)

    readelf -sW $filePath | grep "\bandroid_main_override\b" || (echo "executable or library does not contain android_main1" && return)
    na_stop
    adb shell logcat -c
    adb push $filePath /data/data/com.example.native_activity/files/
    adb shell setprop native_app_path /data/data/com.example.native_activity/files/$file
    adb shell am start com.example.native_activity/android.app.NativeActivity
    adb shell logcat | grep "native_app"
}

if [[ $# != 0 ]]
then
  na_start $1
fi