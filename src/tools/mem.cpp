//
// Created by zhangfuwen on 2022/4/26.
//
#include <iostream>
#include <thread>

int main() {
    volatile int x1 = 5;
    volatile int x2 = 5;
    volatile int x3 = 5;
    volatile int x4 = 5;

    auto thr1 = std::thread([&x1]() {
        while(true) {
            x1 = rand();
        }
    });

    auto thr2 = std::thread([&x2]() {
      while(true) {
          x2 = rand();
      }
    });
    auto thr3 = std::thread([&x3]() {
      while(true) {
          x3 = rand();
      }
    });
    auto thr4 = std::thread([&x4]() {
      while(true) {
          x4 = rand();
      }
    });

    thr1.join();
    thr2.join();
    thr3.join();
    thr4.join();
    return 0;
}