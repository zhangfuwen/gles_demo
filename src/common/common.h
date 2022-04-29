//
// Created by zhangfuwen on 2022/4/28.
//

#ifndef TEST_COMMON_H
#define TEST_COMMON_H
#include <ctime>
#include <memory>
#include <vector>

#define FUN_PRINT(fmt, ...) printf(fmt"\n", ##__VA_ARGS__); fflush(stdout)
#include "handycpp/logging.h"

#ifndef FUN_INFO
#define LOGI(fmt, ...)  printf(fmt"\n", ##__VA_ARGS__); fflush(stdout)
#else
#define LOGI FUN_INFO
#endif


#define LOGE(fmt, ...) LOGI("-----------" fmt " ---------", ##__VA_ARGS__)

#define LOG_TAG "offscreen"
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct defer {
    defer(std::function<void(void)> f) : f_(f) {}
    ~defer(void) { f_(); }

private:
    std::function<void(void)> f_;
};

#endif // TEST_COMMON_H
