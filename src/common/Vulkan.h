//
// Created by zhangfuwen on 2022/4/28.
//

#ifndef TEST_VULKAN_H
#define TEST_VULKAN_H

#define VK_USE_PLATFORM_ANDROID_KHR
#include "../android/Android.h"
#include <vulkan/vulkan.h>

#define CASE_STR(value)                                                                                                \
    case value:                                                                                                        \
        return #value;

inline std::string vkErrString(EGLint error) {
    switch (error) {
        CASE_STR(VK_NOT_READY)
        CASE_STR(VK_TIMEOUT)
        CASE_STR(VK_EVENT_SET)
        CASE_STR(VK_EVENT_RESET)
        CASE_STR(VK_INCOMPLETE)
        CASE_STR(VK_ERROR_OUT_OF_HOST_MEMORY)
        CASE_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY)
        CASE_STR(VK_ERROR_INITIALIZATION_FAILED)
        CASE_STR(VK_ERROR_DEVICE_LOST)
        CASE_STR(VK_ERROR_MEMORY_MAP_FAILED)
        CASE_STR(VK_ERROR_LAYER_NOT_PRESENT)
        CASE_STR(VK_ERROR_EXTENSION_NOT_PRESENT)
        CASE_STR(VK_ERROR_FEATURE_NOT_PRESENT)
        CASE_STR(VK_ERROR_INCOMPATIBLE_DRIVER)
        CASE_STR(VK_ERROR_TOO_MANY_OBJECTS)
        CASE_STR(VK_ERROR_FORMAT_NOT_SUPPORTED)
        CASE_STR(VK_ERROR_FRAGMENTED_POOL)
        CASE_STR(VK_ERROR_OUT_OF_POOL_MEMORY)
        CASE_STR(VK_ERROR_INVALID_EXTERNAL_HANDLE)
        CASE_STR(VK_ERROR_SURFACE_LOST_KHR)
        CASE_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        CASE_STR(VK_SUBOPTIMAL_KHR)
        CASE_STR(VK_ERROR_OUT_OF_DATE_KHR)
        CASE_STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        CASE_STR(VK_ERROR_VALIDATION_FAILED_EXT)
        CASE_STR(VK_ERROR_INVALID_SHADER_NV)
        CASE_STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
        CASE_STR(VK_ERROR_FRAGMENTATION_EXT)
        CASE_STR(VK_ERROR_NOT_PERMITTED_EXT)
        CASE_STR(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT)
        CASE_STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
        CASE_STR(VK_RESULT_RANGE_SIZE)
        CASE_STR(VK_RESULT_MAX_ENUM)
        default:
            std::string s = "Unknown ";
            s += std::to_string(error);
            return s;
    }
}
#undef CASE_STR

#define CALL_VK(func)                                                                                                  \
    if (auto ret = (func); ret != VK_SUCCESS) {                                                                        \
        LOGE("Vulkan error %d, %s", ret, vkErrString(ret).c_str());                                                    \
        assert(false);                                                                                                 \
    }

typedef VkResult (*PFN_vkGetMemoryAndroidHardwareBufferANDROID)(
    VkDevice device,
    const VkMemoryGetAndroidHardwareBufferInfoANDROID *pInfo,
    struct AHardwareBuffer **pBuffer);

#define DEF_VK_FUNC_POINTER(name) PFN_vk##name pfnVk##name = nullptr

#define LIST_VK_POINTERS(macro)                                                                                        \
    macro(GetPhysicalDeviceImageFormatProperties2KHR);                                                                 \
    macro(GetMemoryFdKHR);                                                                                             \
    macro(GetMemoryAndroidHardwareBufferANDROID)

struct VulkanSwapchainInfo {
    VkSwapchainKHR swapchain_;
    uint32_t swapchainLength_;

    VkExtent2D displaySize_;
    VkFormat displayFormat_;

    // array of frame buffers and views
    std::vector<VkImage> displayImages_;
    std::vector<VkImageView> displayViews_;
    std::vector<VkFramebuffer> framebuffers_;
};

class Vulkan {
public:
    int Init();
    int Finish();
    int Draw();

    int CreateMemObjFd(int w, int h, int *size = nullptr);
    bool GetMemoryTypeFromProperties(uint32_t typeBits, VkFlags requirementsMask, uint32_t *typeIndex);
    AHardwareBuffer *ToBuffer();
    void CreateSwapChain(ANativeWindow *aNativeWindow);
    void DeleteSwapChain(void);
    int CreateRenderPass();
    void CreateFrameBuffers(VkRenderPass& renderPass,
                                    VkImageView depthView = VK_NULL_HANDLE);

private:
    VkInstance m_vkInstance = nullptr;
    VkPhysicalDevice m_vkPhysicalDevice = nullptr;
    VkDevice m_vkDevice = nullptr;
    VkQueue m_vkQueue = nullptr;
    uint32_t m_vkQueueFamilyIndex = -1;
    VkDeviceMemory vkDeviceMemory;
    VulkanSwapchainInfo m_swapchain;
    VkSurfaceKHR m_surface;
    VkRenderPass m_vkRenderPass;

    // function pointers
    LIST_VK_POINTERS(DEF_VK_FUNC_POINTER);
    void InitVkPointers();
    int CheckVkPointers();
};

#endif // TEST_VULKAN_H
