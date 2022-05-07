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
#define LOGW LOGI
#define LOGD LOGI
#else
#define LOGI FUN_INFO
#define LOGW FUN_WARN
#endif


#define LOGE(fmt, ...) LOGI("-----------" fmt " ---------", ##__VA_ARGS__)
#if 0
#define LOGD FUN_DEBUG
#else
#define LOGD(...)
#endif

#define LOG_BANNER(fmt, ...) LOGI("###################### " fmt "###################", ##__VA_ARGS__)

#define LOG_TAG "offscreen"
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct defer {
    defer(std::function<void(void)> f) : f_(f) {}
    ~defer(void) { f_(); }

private:
    std::function<void(void)> f_;
};

inline std::string to_string_2(double x) {
    char a[20];
    sprintf(a, "%.2f", x);
    return a;
}

inline std::string hr(uint64_t val) { // human readable
    if (val / 1000000000 != 0) {
        auto ret = double(val) / 1000000000;
        return to_string_2(ret) + " G";
    } else if (val / 1000000 != 0) {
        auto ret = double(val) / 1000000;
        return to_string_2(ret) + " M";
    } else if (val / 1000 != 0) {
        auto ret = double(val) / 1000;
        return to_string_2(ret) + " M";
    } else {
        return std::to_string(val);
    }
}

#endif // TEST_COMMON_H
