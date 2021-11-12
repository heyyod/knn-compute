#include "vulkan_platform.h"

namespace Vulkan
{
    func bool LoadDLL();
    func bool Initialize();
    
    func bool CreatePipeline(pipeline_type pipelineType, u32 **distPerImgData, u64 &distPerImgDataSize);
    func bool CreateBuffer(VkBufferUsageFlags usage, u64 size, VkMemoryPropertyFlags properties,  vulkan_buffer &bufferOut, bool mapBuffer);
    
    func bool UploadInputData(void *testData, void *trainData);
    
    func void ClearPipeline();
    func void ClearBuffer(vulkan_buffer buffer);
    func void Destroy();
    
    func bool Compute(u32 &testImageIndex, u32 distP);
    
    func bool LoadShader(char *filepath, VkShaderModule *shaderOut);
    func bool FindMemoryProperties(u32 memoryType, VkMemoryPropertyFlags requiredProperties, u32 &memoryIndexOut);
    
#if VULKAN_VALIDATION_LAYERS_ON
    func VKAPI_ATTR VkBool32 VKAPI_CALL
        DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                      const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                      void *pUserData);
#endif
}

#if VULKAN_VALIDATION_LAYERS_ON
func VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
              void *pUserData)
{
    bool isError = false;
    char *type;
    if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        type = "Some general event has occurred";
    else if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        type = "Something has occurred during validation against the Vulkan specification that may indicate invalid behavior.";
    else
        type = "Potentially non-optimal use of Vulkan.";
    
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        DebugPrint("WARNING: ");
    }
    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        
    {
        DebugPrint("DIAGNOSTIC: ");
    }
    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        DebugPrint("INFO: ");
    }
    else
    {
        DebugPrint("ERROR: ");
        isError = true;
    }
    DebugPrint(type);
    DebugPrint('\n');
    DebugPrint(callbackData->pMessageIdName);
    DebugPrint('\n');
    
    char c;
    u32 i = 0;
    do
    {
        c = callbackData->pMessage[i];
        i++;
    } while (c != ']');
    i++;
    do
    {
        c = callbackData->pMessage[i];
        if (c != ';' && c != '|')
        {
            DebugPrint(c);
            i++;
        }
        else
        {
            DebugPrint('\n');
            i++;
            do
            {
                c = callbackData->pMessage[i];
                i++;
            } while (c == ' ' || c == '|');
            i--;
        }
    } while (c != '\0');
    DebugPrint('\n');
    if (isError)
    {
        Assert(0);
        return VK_FALSE;
    }
    return VK_FALSE;
}

#endif

func bool Vulkan::
FindMemoryProperties(u32 reqMemType, VkMemoryPropertyFlags reqMemProperties, u32 &memoryIndexOut)
{
    // NOTE(heyyod): We assume we have already set the memory properties during creation.
    u32 memoryCount = vulkan.memoryProperties.memoryTypeCount;
    for (memoryIndexOut = 0; memoryIndexOut < memoryCount; ++memoryIndexOut)
    {
        uint32_t memoryType= (1 << memoryIndexOut);
        bool isRequiredMemoryType = reqMemType & memoryType;
        if(isRequiredMemoryType)
        {
            VkMemoryPropertyFlags properties = vulkan.memoryProperties.memoryTypes[memoryIndexOut].propertyFlags;
            bool hasRequiredProperties = ((properties & reqMemProperties) == reqMemProperties);
            if (hasRequiredProperties)
                return true;
        }
    }
    return false;
}

// TODO: make this cross-platform
func bool Vulkan::
LoadDLL()
{
    vulkan.dll = LoadLibraryA("vulkan-1.dll");
    if (!vulkan.dll)
        return false;
    
    vkGetInstanceProcAddr = (vk_get_instance_proc_addr *)GetProcAddress(vulkan.dll, "vkGetInstanceProcAddr");
    
    DebugPrint("Loaded DLL\n");
    return true;
}


// TODO: Make it cross-platform
func bool Vulkan::
Initialize()
{
    DebugPrint("Initialize Vulkan\n");
    if (!LoadDLL())
    {
        return false;
    }
    
    if (!VulkanLoadGlobalFunctions())
    {
        return false;
    }
    
    // NOTE: Create an instance
    {
#if VULKAN_VALIDATION_LAYERS_ON
        char *instanceLayers[] = {
            "VK_LAYER_KHRONOS_validation"
        };
#else
        char **instanceLayers = 0;
#endif
        
        
        if (instanceLayers)
        {
            u32 layerCount;
            AssertSuccess(vkEnumerateInstanceLayerProperties(&layerCount, 0));
            
            VkLayerProperties availableLayers[16];
            Assert(layerCount <= ArrayCount(availableLayers));
            AssertSuccess(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers));
            
            for (u32 i = 0; i < ArrayCount(instanceLayers); i++)
            {
                char *layerName = instanceLayers[i];
                bool layerFound = false;
                for (VkLayerProperties &layerProperties : availableLayers)
                {
                    if (strcmp(layerName, layerProperties.layerName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }
                if (!layerFound)
                {
                    Assert("ERROR: Layer not found");
                    return false;
                }
            }
        }
        
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "KNN & NC";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "None";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        
        // NOTE: Create an instance and attach extensions
        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;
        if (instanceLayers)
        {
            instanceInfo.enabledLayerCount = ArrayCount(instanceLayers);
            instanceInfo.ppEnabledLayerNames = instanceLayers;
        }
        
#if VULKAN_VALIDATION_LAYERS_ON
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
        debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            ;
        debugMessengerInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            ;
        debugMessengerInfo.pfnUserCallback = DebugCallback;
        DebugPrint("Created Vulkan Debug Messenger\n");
        
        char *desiredInstanceExtensions[] = { 
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        };
        u32 instanceExtensionsCount;
        VkExtensionProperties availableInstanceExtensions[255] = {};
        AssertSuccess(vkEnumerateInstanceExtensionProperties(0, &instanceExtensionsCount, 0));
        Assert(instanceExtensionsCount <= ArrayCount(availableInstanceExtensions));
        AssertSuccess(vkEnumerateInstanceExtensionProperties(0, &instanceExtensionsCount, availableInstanceExtensions));
        for (char *desiredInstanceExtension : desiredInstanceExtensions)
        {
            bool found = false;
            for (VkExtensionProperties &availableExtension : availableInstanceExtensions)
            {
                if (strcmp(desiredInstanceExtension, availableExtension.extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                Assert("ERROR: Requested instance extension not supported");
                return false;
            }
        }
        instanceInfo.enabledExtensionCount = ArrayCount(desiredInstanceExtensions);
        instanceInfo.ppEnabledExtensionNames = desiredInstanceExtensions;
        instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugMessengerInfo;
#endif
        
        AssertSuccess(vkCreateInstance(&instanceInfo, 0, &vulkan.instance));
        DebugPrint("Created Vulkan Instance\n");
        
        if (!VulkanLoadInstanceFunctions())
        {
            Assert("ERROR: Instance Functions not loaded\n");
            return false;
        }
        
#if VULKAN_VALIDATION_LAYERS_ON
        AssertSuccess(vkCreateDebugUtilsMessengerEXT(vulkan.instance, &debugMessengerInfo, 0, &vulkan.debugMessenger));
#endif
    }
    
    // NOTE: Select a gpu that supports compute operations
    {
        u32 gpuCount = 0;
        VkPhysicalDevice gpuBuffer[16] = {};
        AssertSuccess(vkEnumeratePhysicalDevices(vulkan.instance, &gpuCount, 0));
        if(!(gpuCount > 0 && gpuCount <= ArrayCount(gpuBuffer)))
        {
            Print("No GPU Found!\n");
            return false;
        }
        
        /* 
                char *desiredDeviceExtensions[] = {
                    VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME,
                };
         */
        
        AssertSuccess(vkEnumeratePhysicalDevices(vulkan.instance, &gpuCount, gpuBuffer));
        vulkan.computeQueueFamilyIndex = UINT32_MAX;
        for (u8 iGPU = 0; iGPU < gpuCount; iGPU++)
        {
            /* 
                        u32 deviceExtensionsCount;
                        VkExtensionProperties availableDeviceExtensions[255] = {};
                        AssertSuccess(vkEnumerateDeviceExtensionProperties(gpuBuffer[iGPU], 0, &deviceExtensionsCount, 0));
                        Assert(deviceExtensionsCount <= ArrayCount(availableDeviceExtensions));
                        AssertSuccess(vkEnumerateDeviceExtensionProperties(gpuBuffer[iGPU], 0, &deviceExtensionsCount, availableDeviceExtensions));
                        
                        for (char *desiredDeviceExtension : desiredDeviceExtensions)
                        {
                            bool found = false;
                            for (VkExtensionProperties &availableExtension : availableDeviceExtensions)
                            {
                                if (strcmp(desiredDeviceExtension, availableExtension.extensionName) == 0)
                                {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                return false;
                            }
                        }
                         */
            
            {
                // NOTE: Pick a queue family the supports compute operations
                u32 queueFamilyCount;
                VkQueueFamilyProperties availableQueueFamilies[16] = {};
                vkGetPhysicalDeviceQueueFamilyProperties(gpuBuffer[iGPU], &queueFamilyCount, 0);
                Assert(queueFamilyCount <= ArrayCount(availableQueueFamilies));
                vkGetPhysicalDeviceQueueFamilyProperties(gpuBuffer[iGPU], &queueFamilyCount, availableQueueFamilies);
                
                for (u32 i = 0; i < queueFamilyCount; ++i)
                {
                    if ((availableQueueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
                    {
                        if (vulkan.computeQueueFamilyIndex == UINT32_MAX)
                        {
                            vulkan.gpu = gpuBuffer[iGPU];
                            vulkan.computeQueueFamilyIndex = i;
                            break;
                        }
                        
                    }
                }
            }
            
            
        }
        if (vulkan.computeQueueFamilyIndex == UINT32_MAX)
        {
            Print("Couldn't find device that supports compute operations.\n");
            return false;
        }
        
        vkGetPhysicalDeviceProperties(vulkan.gpu, &vulkan.gpuProperties);
        vkGetPhysicalDeviceMemoryProperties(vulkan.gpu, &vulkan.memoryProperties);
        
        
        DebugPrint("Selected a GPU\n");
    }
    
    // NOTE: Create a device
    VkDeviceQueueCreateInfo queueInfo = {};
    {
        float queuePriority = 1.0; // must be array of size queueCount
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = vulkan.computeQueueFamilyIndex;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;
        
        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        AssertSuccess(vkCreateDevice(vulkan.gpu, &deviceInfo, 0, &vulkan.device));
        DebugPrint("Created a Vulkan Device\n");
        
        if (!VulkanLoadDeviceFunctions())
        {
            Assert("ERROR: Device Functions not loaded\n");
            return false;
        }
        vkGetDeviceQueue(vulkan.device, vulkan.computeQueueFamilyIndex, 0, &vulkan.computeQueue);
    }
    
    // NOTE(heyyod): Create Command Buffer
    {
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = vulkan.computeQueueFamilyIndex;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        AssertSuccess(vkCreateCommandPool(vulkan.device, &cmdPoolInfo, 0, &vulkan.cmdPool));
        
        VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
        cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferAllocInfo.commandPool = vulkan.cmdPool;
        cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAllocInfo.commandBufferCount = 1;
        AssertSuccess(vkAllocateCommandBuffers(vulkan.device, &cmdBufferAllocInfo, &vulkan.cmdBuffer));
    }
    
    // NOTE(heyyod): Allocate device local buffers for input data
    {
        u64 trainDataSize = TRAIN_NUM_IMAGES * PIXELS_PER_IMAGE;
        u64 testDataSize = TEST_NUM_IMAGES * PIXELS_PER_IMAGE;
        if (!CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, trainDataSize, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.inTrainBuffer, false) ||
            !CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, testDataSize, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan.inTestBuffer, false))
        {
            return false;
        }
    }
    
    return true;
}

func bool Vulkan::
UploadInputData(void *testData, void *trainData)
{
    u64 trainDataSize = TRAIN_NUM_IMAGES * PIXELS_PER_IMAGE;
    u64 testDataSize = TEST_NUM_IMAGES * PIXELS_PER_IMAGE;
    
    vulkan_buffer trainStagingBuffer = {};
    vulkan_buffer testStagingBuffer = {};
    
    if (!CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, trainDataSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, trainStagingBuffer, true) ||
        !CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, testDataSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, testStagingBuffer, true))
    {
        return false;
    }
    
    memcpy (trainStagingBuffer.data, trainData, trainDataSize);
    memcpy (testStagingBuffer.data, testData, testDataSize);
    
    
    VkCommandBufferBeginInfo cmdBufferBeginInfo= {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    AssertSuccess(vkBeginCommandBuffer(vulkan.cmdBuffer, &cmdBufferBeginInfo));
    
    VkBufferCopy bufferCopyInfo = {};
    bufferCopyInfo.srcOffset = 0;
    bufferCopyInfo.dstOffset = 0;
    bufferCopyInfo.size = trainDataSize;
    vkCmdCopyBuffer(vulkan.cmdBuffer, trainStagingBuffer.handle, vulkan.inTrainBuffer.handle, 1, &bufferCopyInfo);
    
    // NOTE(heyyod): Set where to read and where to copy the vertices
    bufferCopyInfo.srcOffset = 0;
    bufferCopyInfo.dstOffset = 0;
    bufferCopyInfo.size = testDataSize;
    vkCmdCopyBuffer(vulkan.cmdBuffer, testStagingBuffer.handle, vulkan.inTestBuffer.handle, 1, &bufferCopyInfo);
    
    VkBufferMemoryBarrier bufferBarriers[2] = {};
    bufferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarriers[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    bufferBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bufferBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarriers[0].buffer = vulkan.inTrainBuffer.handle;
    bufferBarriers[0].offset = 0;
    bufferBarriers[0].size = VK_WHOLE_SIZE ;
    
    bufferBarriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarriers[1].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    bufferBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT ;
    bufferBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarriers[1].buffer = vulkan.inTestBuffer.handle;
    bufferBarriers[1].offset = 0;
    bufferBarriers[1].size = VK_WHOLE_SIZE ;
    
    vkCmdPipelineBarrier(vulkan.cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, 0, ArrayCount(bufferBarriers), bufferBarriers, 0, 0);
    
    AssertSuccess(vkEndCommandBuffer(vulkan.cmdBuffer));
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
    AssertSuccess(vkQueueSubmit(vulkan.computeQueue, 1, &submitInfo, 0));
    
    vkQueueWaitIdle(vulkan.computeQueue);
    
    ClearBuffer(trainStagingBuffer);
    ClearBuffer(testStagingBuffer);
    
    return true;
}

func bool Vulkan::
CreatePipeline(pipeline_type pipelineType, u32 **distPerImgData, u64 &distPerImgDataSize)
{
    ClearPipeline();
    
    VkShaderModule compShader = {};
    bool loadedShader = false;
    switch (pipelineType)
    {
        case PIPELINE_NEAREST_NEIGHBOUR:
        {
            u64 distPerPixelDataSize = TRAIN_NUM_IMAGES * PIXELS_PER_IMAGE * sizeof(u32);
            distPerImgDataSize = TRAIN_NUM_IMAGES * sizeof(u32);
            
            // NOTE(heyyod): Create buffer for the input and output data
            if (!VulkanIsValidHandle(vulkan.distPerImgBuffer.handle) && !VulkanIsValidHandle(vulkan.distPerPixelBuffer.handle) &&
                CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, distPerPixelDataSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.distPerPixelBuffer, true) &&
                CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, distPerImgDataSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.distPerImgBuffer, true))
            {
                *distPerImgData = (u32 *)vulkan.distPerImgBuffer.data;
            }
            
            // NOTE(heyyod): Create shader bindings
            VkDescriptorSetLayoutBinding bindings[] = {
                {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
            };
            
            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = ArrayCount(bindings);
            layoutInfo.pBindings = bindings;
            AssertSuccess(vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, 0, &vulkan.globalDescSetLayout));
            
            VkDescriptorPoolSize poolSizes[] = {
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, ArrayCount(bindings)},
            };
            
            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = ArrayCount(poolSizes);
            poolInfo.pPoolSizes = poolSizes;
            poolInfo.maxSets = 1;
            AssertSuccess(vkCreateDescriptorPool(vulkan.device, &poolInfo, 0, &vulkan.globalDescPool));
            
            VkDescriptorSetAllocateInfo globalDescAlloc = {};
            globalDescAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            globalDescAlloc.descriptorPool = vulkan.globalDescPool;
            globalDescAlloc.descriptorSetCount = 1;
            globalDescAlloc.pSetLayouts = &vulkan.globalDescSetLayout;
            AssertSuccess(vkAllocateDescriptorSets(vulkan.device, &globalDescAlloc, &vulkan.globalDescSet));
            
            VkDescriptorBufferInfo trainBufferInfo = {};
            trainBufferInfo.buffer = vulkan.inTrainBuffer.handle;
            trainBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet writeTrainBuffer = {};
            writeTrainBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeTrainBuffer.dstSet = vulkan.globalDescSet;
            writeTrainBuffer.dstArrayElement = 0;
            writeTrainBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeTrainBuffer.descriptorCount = 1;
            writeTrainBuffer.dstBinding = 0;
            writeTrainBuffer.pBufferInfo = &trainBufferInfo;
            
            VkDescriptorBufferInfo testBufferInfo = {};
            testBufferInfo.buffer = vulkan.inTestBuffer.handle;
            testBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet writeTestBuffer = {};
            writeTestBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeTestBuffer.dstSet = vulkan.globalDescSet;
            writeTestBuffer.dstArrayElement = 0;
            writeTestBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeTestBuffer.descriptorCount = 1;
            writeTestBuffer.dstBinding = 1;
            writeTestBuffer.pBufferInfo = &testBufferInfo;
            
            VkDescriptorBufferInfo diffPerPixelBufferInfo = {};
            diffPerPixelBufferInfo.buffer = vulkan.distPerPixelBuffer.handle;
            diffPerPixelBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet writeDiffPerPixelBuffer = {};
            writeDiffPerPixelBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDiffPerPixelBuffer.dstSet = vulkan.globalDescSet;
            writeDiffPerPixelBuffer.dstArrayElement = 0;
            writeDiffPerPixelBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeDiffPerPixelBuffer.descriptorCount = 1;
            writeDiffPerPixelBuffer.dstBinding = 2;
            writeDiffPerPixelBuffer.pBufferInfo = &diffPerPixelBufferInfo;
            
            VkDescriptorBufferInfo distPerImgBufferInfo = {};
            distPerImgBufferInfo.buffer = vulkan.distPerImgBuffer.handle;
            distPerImgBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet writeDistPerImgBuffer = {};
            writeDistPerImgBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDistPerImgBuffer.dstSet = vulkan.globalDescSet;
            writeDistPerImgBuffer.dstArrayElement = 0;
            writeDistPerImgBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeDistPerImgBuffer.descriptorCount = 1;
            writeDistPerImgBuffer.dstBinding = 3;
            writeDistPerImgBuffer.pBufferInfo = &distPerImgBufferInfo;
            
            VkWriteDescriptorSet writeSets[] = {
                writeTrainBuffer,
                writeTestBuffer,
                writeDiffPerPixelBuffer,
                writeDistPerImgBuffer
            };
            
            vkUpdateDescriptorSets(vulkan.device, ArrayCount(writeSets), writeSets, 0, 0);
            
            loadedShader = LoadShader("..\\build\\shaders\\NearestNeighbour.comp.spv", &compShader);
        }break;
        
        case PIPELINE_NEAREST_CENTROID:
        {
            loadedShader = LoadShader("..\\build\\shaders\\NearestCentroid.comp.spv", &compShader);
        }break;
    }
    if(!loadedShader)
    {
        Assert("Couldn't load shaders");
        return false;
    }
    
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.module = compShader;
    shaderStageCreateInfo.pName = "main";
    
    VkPushConstantRange pushConstant;
    pushConstant.offset = 0;
    pushConstant.size = 2 * sizeof(u32);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vulkan.globalDescSetLayout ;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    AssertSuccess(vkCreatePipelineLayout(vulkan.device, &pipelineLayoutInfo, 0, &vulkan.pipeline.layout));
    
    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = vulkan.pipeline.layout;
    AssertSuccess(vkCreateComputePipelines(vulkan.device, 0, 1, &pipelineCreateInfo, 0, &vulkan.pipeline.handle));
    DebugPrint("Created Pipeline\n");
    
    vkDestroyShaderModule(vulkan.device, compShader, 0);
    return true;
}

func bool Vulkan::
Compute(u32 &testImageIndex, u32 distP)
{
    // NOTE(heyyod): In glsl the u8 array is "casted" to a uint array, so
    // each element of the array contains 4 pixels. This means tha in one invocation
    // we operate on 4 pixels
    //u64 groupCount = nTrainImages * nPixelsPerImage / 4;
    
    push_constants pc  = {};
    pc.distP = distP;
    pc.testId = testImageIndex;
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    AssertSuccess(vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo));
    vkCmdBindPipeline(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan.pipeline.handle);
    vkCmdBindDescriptorSets(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan.pipeline.layout, 0, 1, &vulkan.globalDescSet, 0, 0);
    vkCmdPushConstants(vulkan.cmdBuffer, vulkan.pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants), &pc);
    vkCmdDispatch(vulkan.cmdBuffer, TRAIN_NUM_IMAGES, 1, 1);
    AssertSuccess(vkEndCommandBuffer(vulkan.cmdBuffer));
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
    AssertSuccess(vkQueueSubmit(vulkan.computeQueue, 1, &submitInfo, 0));
    AssertSuccess(vkQueueWaitIdle(vulkan.computeQueue));
    return true;
}

func bool Vulkan::
LoadShader(char *filepath, VkShaderModule *shaderOut)
{
    
    // NOTE(heyyod): read file code from:
    // https://github.com/Erkaman/vulkan_minimal_compute/blob/master/src/main.cpp
    FILE* fp = fopen(filepath, "rb");
    if (fp == NULL) 
    {
        Print("Could not find or open shader file\n");
    }
    
    // get file size.
    fseek(fp, 0, SEEK_END);
    u64 codeSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    u64 padding = codeSize;
    codeSize = u64(ceil(codeSize / 4.0)) * 4;
    padding = codeSize - padding;
    
    // read file contents.
    u8 *shaderCode = (u8 *)malloc(codeSize);
    fread(shaderCode, codeSize, sizeof(u8), fp);
    fclose(fp);
    
    // data padding. 
    for (u64 i = codeSize - padding; i < codeSize; i++)
    {
        shaderCode[i] = 0;
    }
    
    if(shaderCode)
    {
        VkShaderModuleCreateInfo shaderInfo = {};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = codeSize;
        shaderInfo.pCode = (u32 *)shaderCode;
        VkResult res = vkCreateShaderModule(vulkan.device, &shaderInfo, 0, shaderOut);
        if (res == VK_SUCCESS)
        {
            DebugPrint("Loaded Shader\n");
            return true;
        }
    }
    free(shaderCode);
    return false;
    
}

func void Vulkan::
ClearPipeline()
{
    if(VulkanIsValidHandle(vulkan.device))
    {
        vkDeviceWaitIdle(vulkan.device);
        if (VulkanIsValidHandle(vulkan.pipeline.handle))
        {
            vkDestroyPipeline(vulkan.device, vulkan.pipeline.handle, 0);
            DebugPrint("Cleared Pipeline\n");
        }
        
        if (VulkanIsValidHandle(vulkan.pipeline.layout))
        {
            vkDestroyPipelineLayout(vulkan.device, vulkan.pipeline.layout, 0);
        }
    }
}

func bool Vulkan::
CreateBuffer(VkBufferUsageFlags usage, u64 size, VkMemoryPropertyFlags properties, 
             vulkan_buffer &bufferOut, bool mapBuffer)
{
    // TODO(heyyod): make this able to create multiple buffers of the same type and usage if I pass an array
    if(!(VulkanIsValidHandle(bufferOut.handle)))
    {
        bufferOut.size = size;
        
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = usage;
        bufferInfo.size = size;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        AssertSuccess(vkCreateBuffer(vulkan.device, &bufferInfo, 0, &bufferOut.handle));
        
        VkMemoryRequirements memoryReq= {};
        vkGetBufferMemoryRequirements(vulkan.device, bufferOut.handle, &memoryReq);
        
        u32 memIndex = 0;
        if(Vulkan::FindMemoryProperties(memoryReq.memoryTypeBits, properties, memIndex))
        {
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memoryReq.size;
            allocInfo.memoryTypeIndex = memIndex;
            
            AssertSuccess(vkAllocateMemory(vulkan.device, &allocInfo, 0, &bufferOut.memoryHandle));
            AssertSuccess(vkBindBufferMemory(vulkan.device, bufferOut.handle, bufferOut.memoryHandle, 0));
            
            if(mapBuffer)
                AssertSuccess(vkMapMemory(vulkan.device, bufferOut.memoryHandle, 0, bufferOut.size, 0, &bufferOut.data));
        }
        else
        {
            Assert("Could not allocate vertex buffer memory");
            return false;
        }
    }
    return true;
}

func void Vulkan::
ClearBuffer(vulkan_buffer buffer)
{
    if(VulkanIsValidHandle(vulkan.device))
    {
        if(VulkanIsValidHandle(buffer.handle))
            vkDestroyBuffer(vulkan.device, buffer.handle, 0);
        if(VulkanIsValidHandle(buffer.memoryHandle))
        {
            vkFreeMemory(vulkan.device, buffer.memoryHandle, 0);
        }
    }
}

func void Vulkan::
Destroy()
{
    if (VulkanIsValidHandle(vulkan.device))
    {
        vkDeviceWaitIdle(vulkan.device);
        
        ClearPipeline();
        
        if(VulkanIsValidHandle(vulkan.globalDescSetLayout))
            vkDestroyDescriptorSetLayout(vulkan.device, vulkan.globalDescSetLayout, 0);
        
        ClearBuffer(vulkan.inTrainBuffer);
        ClearBuffer(vulkan.inTestBuffer);
        ClearBuffer(vulkan.distPerPixelBuffer);
        ClearBuffer(vulkan.distPerImgBuffer);
        
        if(VulkanIsValidHandle(vulkan.globalDescPool))
            vkDestroyDescriptorPool(vulkan.device, vulkan.globalDescPool, 0);
        
        if(VulkanIsValidHandle(vulkan.cmdPool))
            vkDestroyCommandPool(vulkan.device, vulkan.cmdPool, 0);
        
        vkDestroyDevice(vulkan.device, 0);
    }
    
    if (VulkanIsValidHandle(vulkan.instance))
    {
#if VULKAN_VALIDATION_LAYERS_ON
        if (VulkanIsValidHandle(vulkan.debugMessenger))
            vkDestroyDebugUtilsMessengerEXT(vulkan.instance, vulkan.debugMessenger, 0);
#endif
        
        vkDestroyInstance(vulkan.instance, 0);
    }
    
    if (vulkan.dll)
        FreeLibrary(vulkan.dll);
    
    DebugPrint("Destroyed Vulkan\n");
    return;
}

#if 0
func bool Vulkan::
Draw(update_data *data)
{
    frame_prep_resource *res = GetNextAvailableResource();
    
    // NOTE: Get the next image that we'll use to create the frame and draw it
    // Signal the semaphore when it's available
    local_ u32 nextImage = 0;
    local_ VkResult result = {}; 
    {
        result = vkAcquireNextImageKHR(vulkan.device, vulkan.swapchain, UINT64_MAX, res->imgAvailableSem, 0, &nextImage);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            if (!CreateSwapchain())
                return false;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            Assert("Could not aquire next image");
            return false;
        }
    }
    
    // NOTE(heyyod): Prepare the frame using the selected resources
    {
        local_ VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        // NOTE(heyyod): THE ORDER OF THESE VALUES MUST BE IDENTICAL
        // TO THE ORDER WE SPECIFIED THE RENDERPASS ATTACHMENTS
        local_ VkClearValue clearValues[2] = {}; // NOTE(heyyod): this is a union
        clearValues[0].color = VulkanClearColor(data->clearColor[0], data->clearColor[1], data->clearColor[2], 0.0f);
        clearValues[1].depthStencil = {1.0f, 0};
        
        local_ VkRenderPassBeginInfo renderpassInfo = {};
        renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpassInfo.renderPass = vulkan.renderPass;
        renderpassInfo.renderArea.extent = vulkan.windowExtent;
        renderpassInfo.clearValueCount = ArrayCount(clearValues);
        renderpassInfo.pClearValues = clearValues;
        renderpassInfo.framebuffer = vulkan.framebuffers[nextImage];
        
        local_ VkViewport viewport = {};
        viewport.width = (f32) vulkan.windowExtent.width;
        viewport.height = (f32) vulkan.windowExtent.height;
        viewport.maxDepth = 1.0f;
        
        local_ VkRect2D scissor = {};
        scissor.extent = vulkan.windowExtent;
        
        local_ VkDeviceSize bufferOffset = 0;
        
        AssertSuccess(vkBeginCommandBuffer(res->cmdBuffer, &commandBufferBeginInfo));
        vkCmdBeginRenderPass(res->cmdBuffer, &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(res->cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(res->cmdBuffer, 0, 1, &scissor);
        
        vkCmdBindVertexBuffers(res->cmdBuffer, 0, 1, &vulkan.vertexBuffer.handle, &bufferOffset);
        vkCmdBindIndexBuffer(res->cmdBuffer, vulkan.indexBuffer.handle, 0, VULKAN_INDEX_TYPE);
        u32 firstIndex = 0;
        u32 indexOffset = 0;
        i32 currentPipelineId = -1;
        u32 transformId = 0;
        for(u32 meshId = 0; meshId < vulkan.loadedMeshCount; meshId++)
        {
            if (currentPipelineId != (i32)vulkan.loadedMesh[meshId].pipelineID)
            {
                currentPipelineId = vulkan.loadedMesh[meshId].pipelineID;
                vkCmdBindPipeline(res->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan.pipeline[currentPipelineId].handle);
                vkCmdBindDescriptorSets(res->cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        vulkan.pipeline[currentPipelineId].layout, 0, 1,
                                        &vulkan.frameData[nextImage].globalDescriptor, 0, 0);
            }
            for(u32 meshInstance = 0; meshInstance < vulkan.loadedMesh[meshId].nInstances; meshInstance++)
            {
                vkCmdPushConstants(res->cmdBuffer, vulkan.pipeline[currentPipelineId].layout, VK_SHADER_STAGE_VERTEX_BIT,
                                   0, sizeof(u32), &transformId);
                vkCmdDrawIndexed(res->cmdBuffer, vulkan.loadedMesh[meshId].nIndices, 1,
                                 firstIndex, indexOffset, 0);
                
                // NOTE(heyyod): THIS ASSUMES THE TRANFORMS ARE ALWAYS LINEARLY SAVED :(
                transformId++;
            }
            firstIndex += vulkan.loadedMesh[meshId].nIndices;
            indexOffset += vulkan.loadedMesh[meshId].nVertices;
        }
        
        vkCmdEndRenderPass(res->cmdBuffer);
        AssertSuccess(vkEndCommandBuffer(res->cmdBuffer));
    }
    
    // NOTE: Wait on imageAvailableSem and submit the command buffer and signal renderFinishedSem
    {
        local_ VkPipelineStageFlags waitDestStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        local_ VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = &waitDestStageMask;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &res->imgAvailableSem;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &res->frameReadySem;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &res->cmdBuffer;
        AssertSuccess(vkQueueSubmit(vulkan.graphicsQueue, 1, &submitInfo, res->fence));
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            if (!CreateSwapchain())
            {
                Assert("Could not recreate after queue sumbit.\n");
                return false;
            }
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            Assert("Couldn't submit draw.\n");
            return false;
        }
    }
    
    // NOTE: Submit image to present when signaled by renderFinishedSem
    {
        local_ VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &res->frameReadySem;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulkan.swapchain;
        presentInfo.pImageIndices = &nextImage;
        result = vkQueuePresentKHR(vulkan.presentQueue, &presentInfo);
        
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            if (!CreateSwapchain())
            {
                Assert("Could not recreate after queue present.\n");
                return false;
            }
        }
        else if (result != VK_SUCCESS)
        {
            Assert("Couldn't present image.\n");
            return false;
        }
        
    }
    
    // NOTE(heyyod): Update data for the engine
    {
        u32 nextUniformBuffer = (nextImage + 1) % NUM_DESCRIPTORS;
        data->newCameraBuffer = vulkan.frameData[nextUniformBuffer].cameraBuffer.data;
        data->newSceneBuffer = vulkan.frameData[nextUniformBuffer].sceneBuffer.data;
    }
    
    return true;
}
#endif
