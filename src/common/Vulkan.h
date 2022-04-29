//
// Created by zhangfuwen on 2022/4/28.
//

#ifndef TEST_VULKAN_H
#define TEST_VULKAN_H

#include <vulkan/vulkan.h>

class Vulkan {
public:
    int Init();
    int Finish();
    int Draw();

    int CreateMemObjFd(int w, int h, int *size);
    bool GetMemoryTypeFromProperties( uint32_t typeBits, VkFlags requirementsMask, uint32_t* typeIndex);

    int Test1();
    int Test2();
    int Test3();
    int Test4();
    int Test5();


private:
    VkInstance m_vkInstance = nullptr;
    VkPhysicalDevice m_vkPhysicalDevice = nullptr;
    VkDevice m_vkDevice = nullptr;
    VkQueue m_vkQueue = nullptr;
};

#endif // TEST_VULKAN_H
