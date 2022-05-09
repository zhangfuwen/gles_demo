//
// Created by zhangfuwen on 2022/4/28.
//

#ifndef TEST_VULKAN_H
#define TEST_VULKAN_H

#include <vulkan/vulkan.h>
#include "../android/Android.h"

typedef struct VkMemoryGetAndroidHardwareBufferInfoANDROID {
    VkStructureType    sType;
    const void*        pNext;
    VkDeviceMemory     memory;
} VkMemoryGetAndroidHardwareBufferInfoANDROID;

typedef VkResult (*PFN_vkGetMemoryAndroidHardwareBufferANDROID)(
    VkDevice                                    device,
    const VkMemoryGetAndroidHardwareBufferInfoANDROID* pInfo,
    struct AHardwareBuffer**                    pBuffer);

#define DEF_VK_FUNC_POINTER(name) PFN_vk##name pfnVk##name = nullptr

#define LIST_VK_POINTERS(macro) \
    macro(GetPhysicalDeviceImageFormatProperties2KHR); \
    macro(GetMemoryFdKHR);      \
    macro(GetMemoryAndroidHardwareBufferANDROID)

class Vulkan {
public:
    int Init();
    int Finish();
    int Draw();

    int CreateMemObjFd(int w, int h, int *size = nullptr);
    bool GetMemoryTypeFromProperties( uint32_t typeBits, VkFlags requirementsMask, uint32_t* typeIndex);
    AHardwareBuffer * ToBuffer();



private:
    VkInstance m_vkInstance = nullptr;
    VkPhysicalDevice m_vkPhysicalDevice = nullptr;
    VkDevice m_vkDevice = nullptr;
    VkQueue m_vkQueue = nullptr;
    VkDeviceMemory vkDeviceMemory;

    // function pointers
    LIST_VK_POINTERS(DEF_VK_FUNC_POINTER);
    void InitVkPointers();
    int CheckVkPointers();
};

#endif // TEST_VULKAN_H
