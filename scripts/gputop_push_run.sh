#!/bin/bash

adb push build-android/gputop /data/
adb shell "/data/gputop -f /sys/kernel/debug/tracing/trace_pipe -t 20"