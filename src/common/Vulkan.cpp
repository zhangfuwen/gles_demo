//
// Created by zhangfuwen on 2022/4/28.
//

#include "Vulkan.h"
#include "common.h"
#include <vector>
int Vulkan::Init() {

    // create instance
    std::vector<const char *> instanceExtensions = {
        "VK_KHR_external_fence_capabilities",
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_external_semaphore_capabilities",
        "VK_KHR_get_physical_device_properties2",
    };
    // instance_extensions.push_back("VK_KHR_surface");
    // instance_extensions.push_back("VK_KHR_android_surface");
    // instance_extensions.push_back("VK_EXT_debug_report");
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "vulkan_test",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "null",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };
    VkInstanceCreateInfo instanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data(),
    };
    vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance);

    InitVkPointers();
    if (auto ret = CheckVkPointers(); ret < 0) {
        LOGE("failed to get all vulkan function pointers");
        return ret;
    }


    // enumerate devices and choose first one
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, nullptr);
    VkPhysicalDevice tmpGpus[gpuCount];
    vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, tmpGpus);
    m_vkPhysicalDevice = tmpGpus[0];

    // enumerate queue families and select a graphics queue
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());
    uint32_t qFIndex;
    for (qFIndex = 0; qFIndex < queueFamilyCount; qFIndex++) {
        if (queueFamilyProperties[qFIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }

    // create a logical device
    std::vector<const char *> deviceExtensions = {
        "VK_KHR_swapchain",
        "VK_KHR_dedicated_allocation",
        "VK_KHR_external_fence",
        "VK_KHR_external_memory",
        "VK_KHR_external_semaphore",
        "VK_KHR_get_memory_requirements2",
        "VK_KHR_external_memory_fd",
        "VK_KHR_external_semaphore_fd",
        "VK_KHR_external_fence_fd",
    };
    float queuePriorities = 0;
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = qFIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities,
    };
    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = nullptr,
    };

    vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_vkDevice);
    vkGetDeviceQueue(m_vkDevice, qFIndex, 0, &m_vkQueue);
    LOGI(
        "m_vkInstance=%p, m_vkPhysicalDevice=%p, m_vkDevice=%p, m_vkQueue=%p",
        m_vkInstance,
        m_vkPhysicalDevice,
        m_vkDevice,
        m_vkQueue);
    return 0;
}
bool Vulkan::GetMemoryTypeFromProperties(uint32_t typeBits, VkFlags requirementsMask, uint32_t *typeIndex) {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &physicalDeviceMemoryProperties);
    if (typeIndex == nullptr) {
        LOGE("GetMemoryTypeFromProperties typeIndex == nullptr");
    }
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

/**
 * Create Vulkan Memory object and export fd
 * @param w
 * @param h
 * @param size
 * @return fd
 */
int Vulkan::CreateMemObjFd(int w, int h, int *size) {
    VkImage vkImage;
    VkDeviceMemory vkDeviceMemory;
    VkDeviceSize vkDeviceSize;
    bool useDedicatedAlloc;

    auto colorImageFormat = (VkFormat)VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R8G8B8A8_UNORM;

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = 0,
        .memoryTypeIndex = 0,
    };

    VkExternalMemoryImageCreateInfoKHR extMemImgCreateInfo = {};
    extMemImgCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR;
    extMemImgCreateInfo.pNext = nullptr;
    extMemImgCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

    VkImageCreateInfo imageCreateInfoDst = {};
    imageCreateInfoDst.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfoDst.pNext = &extMemImgCreateInfo;
    imageCreateInfoDst.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfoDst.format = colorImageFormat;
    imageCreateInfoDst.extent.depth = 1.0f;
    imageCreateInfoDst.extent.height = h;
    imageCreateInfoDst.extent.width = w;
    imageCreateInfoDst.mipLevels = 1; // Don't change this without changing "textureObject->mMips = 1;"

    imageCreateInfoDst.arrayLayers = 1;
    imageCreateInfoDst.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfoDst.tiling = VK_IMAGE_TILING_OPTIMAL;

    imageCreateInfoDst.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    imageCreateInfoDst.flags = 0;

    // Before actually creating the image, we need a few more structures
    VkPhysicalDeviceExternalImageFormatInfoKHR DeviceExternalImageFormatInfo = {};
    DeviceExternalImageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO_KHR;
    DeviceExternalImageFormatInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

    VkPhysicalDeviceImageFormatInfo2KHR DeviceImageFormatInfo = {};
    DeviceImageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2_KHR;
    DeviceImageFormatInfo.format = imageCreateInfoDst.format;
    DeviceImageFormatInfo.type = imageCreateInfoDst.imageType;
    DeviceImageFormatInfo.tiling = imageCreateInfoDst.tiling;
    DeviceImageFormatInfo.usage = imageCreateInfoDst.usage;
    DeviceImageFormatInfo.flags = imageCreateInfoDst.flags;
    DeviceImageFormatInfo.pNext = &DeviceExternalImageFormatInfo;

    VkExternalImageFormatPropertiesKHR ExternalImageFormatProperties = {};
    ExternalImageFormatProperties.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES_KHR;
    VkImageFormatProperties2KHR ImageFormatProperties = {};
    ImageFormatProperties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2_KHR;
    ImageFormatProperties.pNext = &ExternalImageFormatProperties;

    auto err = pfnVkGetPhysicalDeviceImageFormatProperties2KHR(
        m_vkPhysicalDevice, &DeviceImageFormatInfo, &ImageFormatProperties);
    if (VK_SUCCESS != err) {
        LOGE("g_vkGetPhysicalDeviceImageFormatProperties2KHR Failed!!!!!");
    }

    // Check for properties of the memory
    bool isExportable =
        ((ExternalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures &
          VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR) != 0);
    bool canBeOpaque =
        ((ExternalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures &
          VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR) != 0);
    bool RequireDedicatedOnly =
        ((ExternalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures &
          VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_KHR) != 0);

    VkExternalMemoryImageCreateInfoKHR ExternalMemoryImageCreateInfo = {};
    ExternalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR;
    ExternalMemoryImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    imageCreateInfoDst.pNext = &ExternalMemoryImageCreateInfo;

    // Now that other structures are added, create the image
    err = vkCreateImage(m_vkDevice, &imageCreateInfoDst, nullptr, &vkImage);
    if (VK_SUCCESS != err) {
        LOGE("vkCreateImage dst Failed!!!!!");
    }

    LOGI("vkImage: %p", vkImage);
    // Need the memory requirements for the new image
    VkMemoryRequirements vkMemoryRequirements;
    vkGetImageMemoryRequirements(m_vkDevice, vkImage, &vkMemoryRequirements);

    uint32_t MemoryTypeIndex = 0;
    auto pass = GetMemoryTypeFromProperties(
        vkMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &MemoryTypeIndex);
    if (!pass) {
        return -1;
    }

    VkMemoryAllocateInfo memoryAllocateInfoDst = {};
    memoryAllocateInfoDst.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfoDst.pNext = nullptr;
    memoryAllocateInfoDst.memoryTypeIndex = MemoryTypeIndex;
    memoryAllocateInfoDst.allocationSize = vkMemoryRequirements.size;

    VkExportMemoryAllocateInfoKHR extMemCreateInfo = {};
    memset(&extMemCreateInfo, 0, sizeof(VkExportMemoryAllocateInfoKHR));
    extMemCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
    extMemCreateInfo.pNext = nullptr; //  &dedicatedProps;
    extMemCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

    // Set this as next until we require dedicated props
    memoryAllocateInfoDst.pNext = &extMemCreateInfo;

    VkMemoryDedicatedAllocateInfoKHR dedicatedProps = {};
    memset(&dedicatedProps, 0, sizeof(VkMemoryDedicatedAllocateInfoKHR));
    if (RequireDedicatedOnly) {
        dedicatedProps.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
        dedicatedProps.image = vkImage;

        dedicatedProps.pNext = memoryAllocateInfoDst.pNext;
        memoryAllocateInfoDst.pNext = &dedicatedProps;
    }

    err = vkAllocateMemory(m_vkDevice, &memoryAllocateInfoDst, nullptr, &vkDeviceMemory);
    if (VK_SUCCESS != err) {
        LOGE("vkAllocateMemory failed");
        return -1;
    }

    err = vkBindImageMemory(m_vkDevice, vkImage, vkDeviceMemory, 0);
    if (VK_SUCCESS != err) {
        LOGE("vkBindImageMemory failed");
        return -1;
    }

    VkMemoryGetFdInfoKHR getFdInfo = {};
    memset(&getFdInfo, 0, sizeof(VkMemoryGetFdInfoKHR));
    getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    getFdInfo.pNext = nullptr;
    getFdInfo.memory = vkDeviceMemory;
    getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

    int fd = -1;
    err = pfnVkGetMemoryFdKHR(m_vkDevice, &getFdInfo, &fd);
    if (VK_SUCCESS != err) {
        LOGE("vkGetMemoryFdKHR failed");
        return -1;
    }
    if (size != nullptr) {
        *size = vkMemoryRequirements.size;
    }

    vkDestroyImage(m_vkDevice, vkImage, nullptr);
    vkFreeMemory(m_vkDevice, vkDeviceMemory, nullptr);
    return fd;
}

int Vulkan::Finish() { return 0; }
int Vulkan::Draw() { return 0; }

void Vulkan::InitVkPointers() {
#define INIT_POINTER(name)                                                                                             \
    if (pfnVk##name == nullptr) {                                                                                      \
        pfnVk##name = (PFN_vk##name)vkGetInstanceProcAddr(m_vkInstance, "vk" #name);                                   \
    }

    LIST_VK_POINTERS(INIT_POINTER)
#undef INIT_POINTER
}

/**
 * check if there is any vulkan functions pointer uninitialized
 * @return  return -1 if there is any vulkan function pointer uninitialized, return 0 otherwise
 */
int Vulkan::CheckVkPointers() {

    bool hasNull = false;
#define CHECK_POINTER(name)                                                                                            \
    LOGD("%s -> %p", "pfnVk" #name, pfnVk##name);                                                                      \
    if (pfnVk##name == nullptr) {                                                                                      \
        FUN_INFO(#name " is nullptr");                                                                                 \
        hasNull = true;                                                                                                \
    }

    LIST_VK_POINTERS(CHECK_POINTER)

    if (hasNull) {
        return -1;
    }
#undef CHECK_POINTER
    return 0;
}