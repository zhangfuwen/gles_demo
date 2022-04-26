#!/bin/bash

# screen on/off adb shell input keyevent 26

function enable_tracing() {
  echo "echo 1 > /sys/kernel/debug/tracing/events/kgsl/$1/enable"
  echo 1 > /sys/kernel/debug/tracing/events/kgsl/$1/enable
}

function disable_tracing() {
  echo "echo 0 > /sys/kernel/debug/tracing/events/kgsl/$1/enable"
  echo 0 > /sys/kernel/debug/tracing/events/kgsl/$1/enable
}


items="andreno_cmdbatch_retire adreno_preempt_trigger adreno_preempt_done"

function enable_all() {
  for item in $items; do
    enable_tracing "$item"
  done
}

function disable_all() {
  for item in $items; do
    disable_tracing "$item"
  done

}

function read_enable_value() {
  val_str=$(cat $1)
  echo "read $1 value $val_str"
  if [[ $val_str == "1" ]]; then
    return 1
  elif [[ $val_str == "0" ]]; then
    return 0;
  else
    return 2;
  fi
  return 3
}

function set_enable_value() {
  echo $2 > $1
}

declare -A enables
function list_all_enables() {
   find /sys/kernel/debug/ -name enable
}
function backup_all_enables() {
   echo "list_all_enables"
   files=$(list_all_enables)
   echo "done"
   for file in $files; do
     read_enable_value $file
     res=$?
     if [[ $res == 1 || $res == 0 ]]; then
       enables[$file]=$res
       echo "backing $file $res"
     fi
   done
}
function backup_and_disable_all_enables() {
   echo "list_all_enables"
   files=$(list_all_enables)
   echo "done"
   for file in $files; do
     read_enable_value $file
     res=$?
     if [[ $res == 1 || $res == 0 ]]; then
       enables[$file]=$res
       echo "backing $file $res"
       [[ $res == 1 ]] && set_enable_value $file 0
     fi
   done
}

function restore_all_enables() {
  echo "restoring"
  for file in "${!enables[@]}"; do
    echo "$file - ${enables[$file]}";
    set_enable_value $file ${enables[$file]}
  done
  echo "done"
}

backup_and_disable_all_enables

enable_all
echo "echo 1 > /sys/kernel/debug/tracing/tracing_on"
echo 1 > /sys/kernel/debug/tracing/tracing_on
timeout 5 cat /sys/kernel/debug/tracing/trace_pipe > /data/AndroidGpuTop.log
disable_all

restore_all_enables
