/* date = October 25th 2021 6:56 pm */

#ifndef HY3D_VULKAN_FUNCTIONS_H
#define HY3D_VULKAN_FUNCTIONS_H

#define VulkanFuncPtr(funcName) PFN_##funcName
#define VulkanDeclareFunction(funcName) func VulkanFuncPtr(funcName) funcName

//-----------------------------------------------------
//            Global Functions         
//-----------------------------------------------------
#define VK_GET_INSTANCE_PROC_ADDR(name) PFN_vkVoidFunction name(VkInstance instance, const char *pName)
typedef VK_GET_INSTANCE_PROC_ADDR(vk_get_instance_proc_addr);
VK_GET_INSTANCE_PROC_ADDR(vkGetInstanceProcAddrStub) { return 0; }
func  vk_get_instance_proc_addr *vkGetInstanceProcAddr_ = vkGetInstanceProcAddrStub;
#define vkGetInstanceProcAddr vkGetInstanceProcAddr_

VulkanDeclareFunction(vkCreateInstance);
VulkanDeclareFunction(vkEnumerateInstanceLayerProperties);
VulkanDeclareFunction(vkEnumerateInstanceExtensionProperties);

#define VulkanLoadGlobalFunc(funcName)                                     \
funcName = (VulkanFuncPtr(funcName))vkGetInstanceProcAddr(nullptr, #funcName); \
if (!(funcName))                                                       \
{                                                                  \
return false;                                                  \
}
func bool
VulkanLoadGlobalFunctions()
{
    //test
    VulkanLoadGlobalFunc(vkCreateInstance);
    VulkanLoadGlobalFunc(vkEnumerateInstanceLayerProperties);
    VulkanLoadGlobalFunc(vkEnumerateInstanceExtensionProperties);
    
    DebugPrint("Loaded Global Functions\n");
    
    return true;
}

//-----------------------------------------------------
//            Instance Functions         
//-----------------------------------------------------
VulkanDeclareFunction(vkDestroyInstance);

#if VULKAN_VALIDATION_LAYERS_ON
VulkanDeclareFunction(vkCreateDebugUtilsMessengerEXT);
VulkanDeclareFunction(vkDestroyDebugUtilsMessengerEXT);
#endif

VulkanDeclareFunction(vkEnumeratePhysicalDevices);
VulkanDeclareFunction(vkGetPhysicalDeviceQueueFamilyProperties);
VulkanDeclareFunction(vkGetPhysicalDeviceFormatProperties);
VulkanDeclareFunction(vkGetPhysicalDeviceMemoryProperties);
VulkanDeclareFunction(vkGetPhysicalDeviceProperties);
VulkanDeclareFunction(vkEnumerateDeviceExtensionProperties);
VulkanDeclareFunction(vkCreateDevice);
VulkanDeclareFunction(vkGetDeviceProcAddr);

#define VulkanLoadInstanceFunc(funcName)                                           \
funcName = (VulkanFuncPtr(funcName))vkGetInstanceProcAddr(vulkan.instance, #funcName); \
if (!(funcName))                                                               \
{                                                                          \
return false;                                                          \
}
func  bool
VulkanLoadInstanceFunctions()
{
    VulkanLoadInstanceFunc(vkDestroyInstance);
    
#if VULKAN_VALIDATION_LAYERS_ON
    VulkanLoadInstanceFunc(vkCreateDebugUtilsMessengerEXT);
    VulkanLoadInstanceFunc(vkDestroyDebugUtilsMessengerEXT);
#endif
    
    VulkanLoadInstanceFunc(vkEnumeratePhysicalDevices);
    VulkanLoadInstanceFunc(vkGetPhysicalDeviceQueueFamilyProperties);
    VulkanLoadInstanceFunc(vkGetPhysicalDeviceFormatProperties);
    VulkanLoadInstanceFunc(vkGetPhysicalDeviceMemoryProperties);
    VulkanLoadInstanceFunc(vkGetPhysicalDeviceProperties);
    VulkanLoadInstanceFunc(vkEnumerateDeviceExtensionProperties);
    VulkanLoadInstanceFunc(vkCreateDevice);
    VulkanLoadInstanceFunc(vkGetDeviceProcAddr);
    
    DebugPrint("Loaded Instance Functions\n");
    
    return true;
}

//-----------------------------------------------------
//            Device Functions         
//-----------------------------------------------------
VulkanDeclareFunction(vkDeviceWaitIdle);
VulkanDeclareFunction(vkDestroyDevice);
VulkanDeclareFunction(vkGetDeviceQueue);
VulkanDeclareFunction(vkCreateSemaphore);
VulkanDeclareFunction(vkDestroySemaphore);
VulkanDeclareFunction(vkQueueSubmit);
VulkanDeclareFunction(vkQueueWaitIdle);
VulkanDeclareFunction(vkCreateCommandPool);
VulkanDeclareFunction(vkResetCommandPool);
VulkanDeclareFunction(vkDestroyCommandPool);
VulkanDeclareFunction(vkAllocateCommandBuffers);
VulkanDeclareFunction(vkFreeCommandBuffers);
VulkanDeclareFunction(vkBeginCommandBuffer);
VulkanDeclareFunction(vkEndCommandBuffer);
VulkanDeclareFunction(vkResetCommandBuffer);
VulkanDeclareFunction(vkAllocateMemory);
VulkanDeclareFunction(vkFreeMemory);
VulkanDeclareFunction(vkCreateFence);
VulkanDeclareFunction(vkResetFences);
VulkanDeclareFunction(vkDestroyFence);
VulkanDeclareFunction(vkWaitForFences);
VulkanDeclareFunction(vkCreateShaderModule);
VulkanDeclareFunction(vkDestroyShaderModule);
VulkanDeclareFunction(vkCreatePipelineLayout);
VulkanDeclareFunction(vkDestroyPipelineLayout);
VulkanDeclareFunction(vkCreateGraphicsPipelines);
VulkanDeclareFunction(vkDestroyPipeline);
VulkanDeclareFunction(vkCreateBuffer);
VulkanDeclareFunction(vkDestroyBuffer);
VulkanDeclareFunction(vkGetBufferMemoryRequirements);
VulkanDeclareFunction(vkBindBufferMemory);
VulkanDeclareFunction(vkMapMemory);
VulkanDeclareFunction(vkUnmapMemory);
VulkanDeclareFunction(vkFlushMappedMemoryRanges);
VulkanDeclareFunction(vkCreateDescriptorSetLayout);
VulkanDeclareFunction(vkDestroyDescriptorSetLayout);
VulkanDeclareFunction(vkCreateDescriptorPool);
VulkanDeclareFunction(vkDestroyDescriptorPool);
VulkanDeclareFunction(vkAllocateDescriptorSets);
VulkanDeclareFunction(vkUpdateDescriptorSets);
VulkanDeclareFunction(vkCreateComputePipelines);

VulkanDeclareFunction(vkCmdPipelineBarrier);
VulkanDeclareFunction(vkCmdBindPipeline);
VulkanDeclareFunction(vkCmdBindDescriptorSets);
VulkanDeclareFunction(vkCmdPushConstants);
VulkanDeclareFunction(vkCmdDispatch);
VulkanDeclareFunction(vkCmdCopyBuffer);

#define VulkanLoadDeviceFunc(funcName)                                         \
funcName = (VulkanFuncPtr(funcName))vkGetDeviceProcAddr(vulkan.device, #funcName); \
if (!(funcName))                                                           \
{                                                                      \
return false;                                                      \
}
func  bool
VulkanLoadDeviceFunctions()
{
    VulkanLoadDeviceFunc(vkDeviceWaitIdle);
    VulkanLoadDeviceFunc(vkDestroyDevice);
    VulkanLoadDeviceFunc(vkGetDeviceQueue);
    VulkanLoadDeviceFunc(vkCreateSemaphore);
    VulkanLoadDeviceFunc(vkDestroySemaphore);
    VulkanLoadDeviceFunc(vkQueueSubmit);
    VulkanLoadDeviceFunc(vkQueueWaitIdle);
    VulkanLoadDeviceFunc(vkCreateCommandPool);
    VulkanLoadDeviceFunc(vkResetCommandPool);
    VulkanLoadDeviceFunc(vkDestroyCommandPool);
    VulkanLoadDeviceFunc(vkAllocateCommandBuffers);
    VulkanLoadDeviceFunc(vkFreeCommandBuffers);
    VulkanLoadDeviceFunc(vkBeginCommandBuffer);
    VulkanLoadDeviceFunc(vkEndCommandBuffer);
    VulkanLoadDeviceFunc(vkResetCommandBuffer);
    VulkanLoadDeviceFunc(vkAllocateMemory);
    VulkanLoadDeviceFunc(vkFreeMemory);
    VulkanLoadDeviceFunc(vkCreateFence);
    VulkanLoadDeviceFunc(vkDestroyFence);
    VulkanLoadDeviceFunc(vkWaitForFences);
    VulkanLoadDeviceFunc(vkResetFences);
    VulkanLoadDeviceFunc(vkCreateShaderModule);
    VulkanLoadDeviceFunc(vkDestroyShaderModule);
    VulkanLoadDeviceFunc(vkCreatePipelineLayout);
    VulkanLoadDeviceFunc(vkDestroyPipelineLayout);
    VulkanLoadDeviceFunc(vkCreateGraphicsPipelines);
    VulkanLoadDeviceFunc(vkDestroyPipeline);
    VulkanLoadDeviceFunc(vkCreateBuffer);
    VulkanLoadDeviceFunc(vkDestroyBuffer);
    VulkanLoadDeviceFunc(vkBindBufferMemory);
    VulkanLoadDeviceFunc(vkMapMemory);
    VulkanLoadDeviceFunc(vkUnmapMemory);
    VulkanLoadDeviceFunc(vkGetBufferMemoryRequirements);
    VulkanLoadDeviceFunc(vkFlushMappedMemoryRanges);
    VulkanLoadDeviceFunc(vkCreateDescriptorSetLayout);
    VulkanLoadDeviceFunc(vkDestroyDescriptorSetLayout);
    VulkanLoadDeviceFunc(vkCreateDescriptorPool);
    VulkanLoadDeviceFunc(vkDestroyDescriptorPool);
    VulkanLoadDeviceFunc(vkAllocateDescriptorSets);
    VulkanLoadDeviceFunc(vkUpdateDescriptorSets);
    VulkanLoadDeviceFunc(vkCreateComputePipelines);
    
    VulkanLoadDeviceFunc(vkCmdPipelineBarrier);
    VulkanLoadDeviceFunc(vkCmdBindPipeline);
    VulkanLoadDeviceFunc(vkCmdBindDescriptorSets);
    VulkanLoadDeviceFunc(vkCmdPushConstants);
    VulkanLoadDeviceFunc(vkCmdDispatch);
    VulkanLoadDeviceFunc(vkCmdCopyBuffer);
    
    DebugPrint("Loaded Device Functions\n");
    
    return true;
}

#endif //HY3D_VULKAN_FUNCTIONS_H
