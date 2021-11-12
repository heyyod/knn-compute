#ifndef HY3D_VULKAN_H
#define HY3D_VULKAN_H 1

#include "hy3d_base.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include "vulkan\vulkan.h"

#define SHADER_CODE_BUFFER_SIZE 4096
#define WORKGROUP_SIZE 256
#define MAP_BUFFER_TRUE true

struct vulkan_buffer
{
    VkBuffer handle;
    VkDeviceMemory memoryHandle;
    void *data;
    u64 size;
    u64 writeOffset;
};

struct vulkan_pipeline
{
    VkPipeline handle;
    VkPipelineLayout layout;
};

struct push_constants
{
    u32 distP;
    u32 testId;
};

enum pipeline_type
{
    PIPELINE_NEAREST_NEIGHBOUR,
    PIPELINE_NEAREST_CENTROID,
    PIPELINE_DISTANCES_CALC
};

struct vulkan_engine
{
    VkPhysicalDeviceProperties gpuProperties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    
    VkQueue computeQueue;
    u32 computeQueueFamilyIndex;
    
    VkDescriptorSetLayout globalDescSetLayout;
    VkDescriptorPool globalDescPool;
    VkDescriptorSet globalDescSet;
    
    vulkan_pipeline pipeline;
    
    vulkan_buffer inTrainBuffer;
    vulkan_buffer inTestBuffer;
    vulkan_buffer distPerPixelBuffer;
    vulkan_buffer distPerImgBuffer;
    
    // NOTE(heyyod): We have a set of resources that we use in a circular way to prepare
    // the next frame. For now the number of resources is the same as the swapchain images.
    // THIS IS IMPORTANT. THE FRAMEBUFFER CREATION DEPENDS ON THIS!!!
    VkCommandPool cmdPool;
    VkCommandBuffer cmdBuffer;
    
#if VULKAN_VALIDATION_LAYERS_ON
    VkDebugUtilsMessengerEXT debugMessenger;
#endif
    
#ifdef VK_USE_PLATFORM_WIN32_KHR
    HMODULE dll;
#endif
};

global_var vulkan_engine vulkan;

#include "vulkan_functions.h"

//-
// NOTE: Macros

#if HY3D_DEBUG
#define AssertSuccess(FuncResult) \
if (FuncResult != VK_SUCCESS)     \
{                                 \
DebugPrint(FuncResult);      \
AssertBreak();                \
}
#else
#define AssertSuccess(FuncResult) \
if (FuncResult != VK_SUCCESS)     \
return false;
#endif

#define VulkanClearColor(r, g, b, a) \
{                                \
r, g, b, a                   \
}
#define VulkanIsValidHandle(handle) (handle != VK_NULL_HANDLE)

#endif
