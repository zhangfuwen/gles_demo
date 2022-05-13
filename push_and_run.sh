#!/bin/bash

executable=$1
if adb shell pm list packages | grep com.your.app.package; then
  echo "package already exists"
else
  echo "package not exists, download and installing"
  wget https://github.com/zhangfuwen/native_app/releases/download/v1.0.0/app-debug.apk
  adb install app-debug.apk
fi

dir=$(dirname $executable)
file=$(basename $executable)
echo "dir:[$dir], file:[$file]"
adb push $executable /data/local/tmp/
adb shell /data/local/tmp/$file