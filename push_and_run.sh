#!/bin/bash

executable=$1


dir=$(dirname $executable)
file=$(basename $executable)
echo "dir:[$dir], file:[$file]"
adb push $executable /data/local/tmp/
adb shell /data/local/tmp/$file