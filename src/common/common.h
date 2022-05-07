//
// Created by zhangfuwen on 2022/4/28.
//

#ifndef TEST_COMMON_H
#define TEST_COMMON_H
#include <ctime>
#include <memory>
#include <vector>
#include <android/log.h>

//#define FUN_PRINT(fmt, ...) printf(fmt"\n", ##__VA_ARGS__); fflush(stdout)
#define FUN_PRINT(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native_app", __VA_ARGS__))
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
#define LOGD FUN_INFO
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

#include <memory>
#include <string>
#include <stdexcept>

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

struct measure_time {
    measure_time(std::function<void(void)> f) {
        timespec t1{}, t2{};
        clock_gettime(CLOCK_MONOTONIC, &t1);
        f();
        clock_gettime(CLOCK_MONOTONIC, &t2);
        if(t2.tv_nsec >= t1.tv_nsec) {
            time_diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
            time_diff.tv_sec = t2.tv_sec - t1.tv_sec;
        } else {
            time_diff.tv_nsec = t2.tv_nsec + 1000000000 - t1.tv_nsec;
            time_diff.tv_sec = t2.tv_sec - t1.tv_sec -1;
        }
    }
    void print_time_ns(std::string prefix = "") {
        LOGI("%s: %us %uns", prefix.c_str(), time_diff.tv_sec, time_diff.tv_nsec);
    }
    void print_time_us(std::string prefix = "") {
            LOGI("%s: %us %uus", prefix.c_str(), time_diff.tv_sec, time_diff.tv_nsec/1000);
    }
    void print_time_ms(std::string prefix = "") {
            LOGI("%s: %us %ums", prefix.c_str(), time_diff.tv_sec, time_diff.tv_nsec/1000000);
    }
    timespec get_time() { return time_diff; }
private:
    timespec time_diff;

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
