#include "vulkan_platform.h"

namespace Vulkan
{
    func bool LoadDLL();
    func bool Initialize();
    
    func bool CreatePipeline(pipeline_type pipelineType);
    func bool CreateBuffer(VkBufferUsageFlags usage, u64 size, VkMemoryPropertyFlags properties,  vulkan_buffer &bufferOut, bool mapBuffer);
    
    func bool UploadInputData(void *testData, void *trainData);
    func bool AllocateKnnMemory(u32 **distPerImgData, u64 &distPerImgDataSize);
    func bool AllocateNeuralNetMemory(u32* layersDims, u32 nLayers, f32 **outWeights, f32 **outBiases, f32 **outValues, f32 **outErrors, f32 **outProducts);
    func void GetGroupCountAndBatches(u32 totalInvocations, u32 groupSize, u32 &groupCount, u32 &batches);
    
    func void ClearPipelinesAndStorageBuffers();
    func void ClearBuffer(vulkan_buffer &buffer);
    func void Destroy();
    
    func bool KnnCompute(u32 &testImageIndex, u32 distP);
    func bool FeedForwardCompute(u32 inValuesIndex, u32 inValuesDim, u32 weightsIndex, u32 weightsDim, u32 biasesIndex, u32 outValuesIndex, u32 outValuesDim);
    func bool BackPropagateCompute(u32 currLayerValuesIndex, u32 prevLayerValuesIndex, u32 inErrorsIndex, u32 inErrorsDim, u32 weightsIndex, u32 weightsDim, u32 biasesIndex, u32 outErrorsIndex, u32 outErrorsDim, f32 learningRate, u32 layerIndex);
    
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
        
        AssertSuccess(vkEnumeratePhysicalDevices(vulkan.instance, &gpuCount, gpuBuffer));
        vulkan.computeQueueFamilyIndex = UINT32_MAX;
        for (u8 iGPU = 0; iGPU < gpuCount; iGPU++)
        {
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
    
    return true;
}

func bool Vulkan::
UploadInputData(void *testData, void *trainData)
{
    u64 trainDataSize = NUM_TRAIN_IMAGES * PIXELS_PER_IMAGE;
    u64 testDataSize = NUM_TEST_IMAGES * PIXELS_PER_IMAGE;
    
    vulkan_buffer inputStagingBuffer = {};
    
    if (!CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, trainDataSize + testDataSize,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, inputStagingBuffer, true))
    {
        return false;
    }
    
    memcpy (inputStagingBuffer.data, trainData, trainDataSize);
    memcpy ((u8 *)inputStagingBuffer.data + trainDataSize, testData, testDataSize);
    
    VkCommandBufferBeginInfo cmdBufferBeginInfo= {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    AssertSuccess(vkBeginCommandBuffer(vulkan.cmdBuffer, &cmdBufferBeginInfo));
    
    VkBufferCopy bufferCopyInfo = {};
    bufferCopyInfo.srcOffset = 0;
    bufferCopyInfo.dstOffset = 0;
    bufferCopyInfo.size = trainDataSize + testDataSize;
    vkCmdCopyBuffer(vulkan.cmdBuffer, inputStagingBuffer.handle, vulkan.valuesBuffer.handle, 1, &bufferCopyInfo);
    
    VkBufferMemoryBarrier bufferBarriers[1] = {};
    bufferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarriers[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    bufferBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bufferBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarriers[0].buffer = vulkan.valuesBuffer.handle;
    bufferBarriers[0].offset = 0;
    bufferBarriers[0].size = VK_WHOLE_SIZE ;
    
    vkCmdPipelineBarrier(vulkan.cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, 0, ArrayCount(bufferBarriers), bufferBarriers, 0, 0);
    
    AssertSuccess(vkEndCommandBuffer(vulkan.cmdBuffer));
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
    AssertSuccess(vkQueueSubmit(vulkan.computeQueue, 1, &submitInfo, 0));
    
    vkQueueWaitIdle(vulkan.computeQueue);
    
    ClearBuffer(inputStagingBuffer);
    
    return true;
}

func bool Vulkan::
AllocateNeuralNetMemory(u32* layersDims, u32 nLayers, f32 **outWeights, f32 **outBiases, f32 **outValues, f32 **outErrors, f32 **outProducts)
{
    Assert(nLayers >= 3);
    
    u64 valuesSize = (NUM_TRAIN_IMAGES + NUM_TEST_IMAGES) * PIXELS_PER_IMAGE;
    u64 biasesSize = 0;
    u64 weightsSize = 0;
    u64 productsSize = 0;
    
    for (u32 i = 1; i < nLayers; i++)
    {
        valuesSize += layersDims[i];
        biasesSize += layersDims[i];
        weightsSize += layersDims[i] * layersDims[i-1];
        u64 count = layersDims[i] * layersDims[i-1] * layersDims[i-1];
        if (count > productsSize)
            productsSize = count;
    }
    valuesSize *= sizeof(f32);
    productsSize *= sizeof(f32);
    biasesSize *= sizeof(f32);
    weightsSize *= sizeof(f32);
    u64 errorsSize = biasesSize;
    
    // NOTE(heyyod): The weighted vals buffer is recyclable, meaning we constatly save the product
    // values of a layer given the weights of the next layer. After that we sum them in the weightsBuffer
    
    if (CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, valuesSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.valuesBuffer, true) &&
        CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, biasesSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.biasesBuffer, true) &&
        CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, weightsSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.weightsBuffer, true) &&
        CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, productsSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.productsBuffer, true) &&
        CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, errorsSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.errorsBuffer, true))
    {
        *outWeights = (f32 *)vulkan.weightsBuffer.data;
        *outBiases = (f32 *)vulkan.biasesBuffer.data;
        *outValues = (f32 *)vulkan.valuesBuffer.data;
        *outErrors = (f32 *)vulkan.errorsBuffer.data;
        *outProducts = (f32 *)vulkan.productsBuffer.data;
        
        return true;
    }
    return false;
}

func bool Vulkan::
AllocateKnnMemory(u32 **distPerImgData, u64 &distPerImgDataSize)
{
    u64 distPerPixelDataSize = NUM_TRAIN_IMAGES * PIXELS_PER_IMAGE * sizeof(u32);
    distPerImgDataSize = NUM_TRAIN_IMAGES * sizeof(u32);
    
    // NOTE(heyyod): Create buffer for the input and output data
    if (CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, distPerPixelDataSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.distPerPixelBuffer, true) &&
        CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, distPerImgDataSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkan.distPerImgBuffer, true))
    {
        *distPerImgData = (u32 *)vulkan.distPerImgBuffer.data;
        return true;
    }
    return false;
}

func bool Vulkan::
CreatePipeline(pipeline_type pipelineType)
{
    if (VulkanIsValidHandle(vulkan.pipelines[pipelineType].handle))
        return true;
    
    VkShaderModule compShader = {};
    bool loadedShader = false;
    
    VkPushConstantRange pushConstant;
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstant.offset = 0;
    
    switch (pipelineType)
    {
        case PIPELINE_TYPE_NEAREST_NEIGHBOUR:
        {
            // TODO(heyyod): I MADE THE INPUT A SINGLE BUFFER FOR TRAIN AND TEST DATA
            // HAVE NOT TESTED IF K-NN STILL WORKS
            // NOTE(heyyod): Create shader bindings
            VkDescriptorSetLayoutBinding bindings[] = {
                {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
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
            
            VkDescriptorBufferInfo inputBufferInfo = {};
            inputBufferInfo.buffer = vulkan.valuesBuffer.handle;
            inputBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet writeInputBuffer = {};
            writeInputBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInputBuffer.dstSet = vulkan.globalDescSet;
            writeInputBuffer.dstArrayElement = 0;
            writeInputBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeInputBuffer.descriptorCount = 1;
            writeInputBuffer.dstBinding = 0;
            writeInputBuffer.pBufferInfo = &inputBufferInfo;
            
            VkDescriptorBufferInfo diffPerPixelBufferInfo = {};
            diffPerPixelBufferInfo.buffer = vulkan.distPerPixelBuffer.handle;
            diffPerPixelBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet writeDiffPerPixelBuffer = {};
            writeDiffPerPixelBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDiffPerPixelBuffer.dstSet = vulkan.globalDescSet;
            writeDiffPerPixelBuffer.dstArrayElement = 0;
            writeDiffPerPixelBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeDiffPerPixelBuffer.descriptorCount = 1;
            writeDiffPerPixelBuffer.dstBinding = 1;
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
            writeDistPerImgBuffer.dstBinding = 2;
            writeDistPerImgBuffer.pBufferInfo = &distPerImgBufferInfo;
            
            VkWriteDescriptorSet writeSets[] = {
                writeInputBuffer,
                writeDiffPerPixelBuffer,
                writeDistPerImgBuffer
            };
            
            vkUpdateDescriptorSets(vulkan.device, ArrayCount(writeSets), writeSets, 0, 0);
            pushConstant.size = sizeof(push_constants_knn);
            loadedShader = LoadShader("..\\build\\shaders\\NearestNeighbour.comp.spv", &compShader);
        }break;
        
        case PIPELINE_TYPE_FEED_FORWARD:
        {
            // NOTE(heyyod): Create shader bindings
            VkDescriptorSetLayoutBinding bindings[] = {
                {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
                {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
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
            
            VkDescriptorBufferInfo inputBufferInfo = {};
            inputBufferInfo.buffer = vulkan.valuesBuffer.handle;
            inputBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet inputBuffer = {};
            inputBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            inputBuffer.dstSet = vulkan.globalDescSet;
            inputBuffer.dstArrayElement = 0;
            inputBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            inputBuffer.descriptorCount = 1;
            inputBuffer.dstBinding = 0;
            inputBuffer.pBufferInfo = &inputBufferInfo;
            
            VkDescriptorBufferInfo weightsBufferInfo = {};
            weightsBufferInfo.buffer = vulkan.weightsBuffer.handle;
            weightsBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet weightsBuffer = {};
            weightsBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            weightsBuffer.dstSet = vulkan.globalDescSet;
            weightsBuffer.dstArrayElement = 0;
            weightsBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            weightsBuffer.descriptorCount = 1;
            weightsBuffer.dstBinding = 1;
            weightsBuffer.pBufferInfo = &weightsBufferInfo;
            
            VkDescriptorBufferInfo biasesBufferInfo = {};
            biasesBufferInfo.buffer = vulkan.biasesBuffer.handle;
            biasesBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet biasesBuffer = {};
            biasesBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            biasesBuffer.dstSet = vulkan.globalDescSet;
            biasesBuffer.dstArrayElement = 0;
            biasesBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            biasesBuffer.descriptorCount = 1;
            biasesBuffer.dstBinding = 2;
            biasesBuffer.pBufferInfo = &biasesBufferInfo;
            
            VkDescriptorBufferInfo productsBufferInfo = {};
            productsBufferInfo.buffer = vulkan.productsBuffer.handle;
            productsBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet productsBuffer = {};
            productsBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            productsBuffer.dstSet = vulkan.globalDescSet;
            productsBuffer.dstArrayElement = 0;
            productsBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            productsBuffer.descriptorCount = 1;
            productsBuffer.dstBinding = 3;
            productsBuffer.pBufferInfo = &productsBufferInfo;
            
            VkDescriptorBufferInfo errorsBufferInfo = {};
            errorsBufferInfo.buffer = vulkan.errorsBuffer.handle;
            errorsBufferInfo.range = VK_WHOLE_SIZE;
            VkWriteDescriptorSet errorsBuffer = {};
            errorsBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            errorsBuffer.dstSet = vulkan.globalDescSet;
            errorsBuffer.dstArrayElement = 0;
            errorsBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            errorsBuffer.descriptorCount = 1;
            errorsBuffer.dstBinding = 4;
            errorsBuffer.pBufferInfo = &errorsBufferInfo;
            
            VkWriteDescriptorSet writeSets[] = {
                inputBuffer,
                weightsBuffer,
                biasesBuffer,
                productsBuffer,
                errorsBuffer
            };
            
            vkUpdateDescriptorSets(vulkan.device, ArrayCount(writeSets), writeSets, 0, 0);
            pushConstant.size = sizeof(push_constants_feed_forward);
            loadedShader = LoadShader("..\\build\\shaders\\FeedForward.comp.spv", &compShader);
        }break;
        
        case PIPELINE_TYPE_BACK_PROPAGATE:
        {
            Assert(VulkanIsValidHandle(vulkan.pipelines[PIPELINE_TYPE_FEED_FORWARD].handle));
            pushConstant.size = sizeof(push_constants_back_propagate);
            loadedShader = LoadShader("..\\build\\shaders\\BackPropagate.comp.spv", &compShader);
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
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vulkan.globalDescSetLayout ;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    AssertSuccess(vkCreatePipelineLayout(vulkan.device, &pipelineLayoutInfo, 0, &vulkan.pipelines[pipelineType].layout));
    
    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = vulkan.pipelines[pipelineType].layout;
    AssertSuccess(vkCreateComputePipelines(vulkan.device, 0, 1, &pipelineCreateInfo, 0, &vulkan.pipelines[pipelineType].handle));
    DebugPrint("Created Pipeline\n");
    
    vkDestroyShaderModule(vulkan.device, compShader, 0);
    return true;
}

func void Vulkan::
GetGroupCountAndBatches(u32 totalInvocations, u32 groupSize, u32 &groupCount, u32 &batches)
{
    u32 totalGroupCount = totalInvocations / groupSize;
    if (totalGroupCount == 0)
        totalGroupCount = totalInvocations;
    else
    {
        while (totalGroupCount * groupSize < totalInvocations)
        {
            totalGroupCount++;
        }
    }
    
    u32 maxGroupCount = vulkan.gpuProperties.limits.maxComputeWorkGroupCount[0];
    batches = totalGroupCount / maxGroupCount;
    
    if (batches == 0)
    {
        groupCount = totalGroupCount;
        batches = 1;
    }
    else
    {
        if (totalGroupCount % maxGroupCount > 0)
            batches++;
        groupCount = vulkan.gpuProperties.limits.maxComputeWorkGroupCount[0];
        
        while (groupCount * batches * groupSize < totalInvocations)
            batches++;
        
        while (groupCount * batches * groupSize > totalInvocations)
            groupCount--;
        while (groupCount * batches * groupSize < totalInvocations)
            groupCount++;
    }
}

func bool Vulkan::
KnnCompute(u32 &testImageIndex, u32 distP)
{
    // NOTE(heyyod): In glsl the u8 array is "casted" to a uint array, so
    // each element of the array contains 4 pixels. This means that in one invocation
    // we operate on 4 pixels and so we need 784/4=196 invocations per image.
    // So the total number of workgroups is the number of images we test against. 
    u32 groupCount = NUM_TRAIN_IMAGES;
    
    push_constants_knn pc  = {};
    pc.testId = testImageIndex;
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    AssertSuccess(vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo));
    vkCmdBindPipeline(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan.pipelines[PIPELINE_TYPE_NEAREST_NEIGHBOUR].handle);
    vkCmdBindDescriptorSets(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan.pipelines[PIPELINE_TYPE_NEAREST_NEIGHBOUR].layout, 0, 1, &vulkan.globalDescSet, 0, 0);
    vkCmdPushConstants(vulkan.cmdBuffer, vulkan.pipelines[PIPELINE_TYPE_NEAREST_NEIGHBOUR].layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
    vkCmdDispatch(vulkan.cmdBuffer, groupCount, 1, 1);
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
FeedForwardCompute(u32 inValuesIndex, u32 inValuesDim, u32 weightsIndex, u32 weightsDim,
                   u32 biasesIndex, u32 outValuesIndex, u32 outValuesDim)
{
    u32 totalInvocations = inValuesDim * outValuesDim;
    u32 groupCount;
    u32 batches;
    GetGroupCountAndBatches(totalInvocations, 256, groupCount, batches);
    
    push_constants_feed_forward pc = {};
    pc.inValuesIndex = inValuesIndex;
    pc.inValuesDim = inValuesDim;
    pc.weightsIndex = weightsIndex;
    pc.weightsDim = weightsDim;
    pc.biasesIndex = biasesIndex;
    pc.outValuesIndex = outValuesIndex;
    pc.outValuesDim = outValuesDim;
    pc.maxBatches = batches;
    
    for (u32 batch = 0; batch < batches; batch++)
    {
        pc.batch = batch;
        
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        AssertSuccess(vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo));
        vkCmdBindPipeline(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan.pipelines[PIPELINE_TYPE_FEED_FORWARD].handle);
        vkCmdBindDescriptorSets(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan.pipelines[PIPELINE_TYPE_FEED_FORWARD].layout, 0, 1, &vulkan.globalDescSet, 0, 0);
        vkCmdPushConstants(vulkan.cmdBuffer, vulkan.pipelines[PIPELINE_TYPE_FEED_FORWARD].layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
        vkCmdDispatch(vulkan.cmdBuffer, groupCount, 1, 1);
        AssertSuccess(vkEndCommandBuffer(vulkan.cmdBuffer));
        
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
        AssertSuccess(vkQueueSubmit(vulkan.computeQueue, 1, &submitInfo, 0));
        AssertSuccess(vkQueueWaitIdle(vulkan.computeQueue));
    }
    return true;
}

func bool Vulkan::
BackPropagateCompute(u32 currLayerValuesIndex, u32 prevLayerValuesIndex,
                     u32 inErrorsIndex, u32 inErrorsDim, u32 weightsIndex, u32 weightsDim, u32 biasesIndex,
                     u32 outErrorsIndex, u32 outErrorsDim, f32 learningRate, u32 layerIndex)
{
    u32 totalInvocations = inErrorsDim * outErrorsDim;
    u32 groupCount;
    u32 batches;
    GetGroupCountAndBatches(totalInvocations, 256, groupCount, batches);
    
    push_constants_back_propagate pc = {};
    pc.currLayerValuesIndex = currLayerValuesIndex;
    pc.prevLayerValuesIndex = prevLayerValuesIndex;
    pc.inErrorsIndex = inErrorsIndex;
    pc.inErrorsDim = inErrorsDim;
    pc.weightsIndex = weightsIndex;
    pc.weightsDim = weightsDim;
    pc.biasesIndex = biasesIndex;
    pc.outErrorsIndex = outErrorsIndex;
    pc.outErrorsDim = outErrorsDim;
    pc.learningRate = learningRate;
    pc.layerIndex = layerIndex;
    pc.maxBatches = batches;
    
    for (u32 batch = 0; batch < batches; batch++)
    {
        pc.batch = batch;
        
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        AssertSuccess(vkBeginCommandBuffer(vulkan.cmdBuffer, &beginInfo));
        vkCmdBindPipeline(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan.pipelines[PIPELINE_TYPE_BACK_PROPAGATE].handle);
        vkCmdBindDescriptorSets(vulkan.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkan.pipelines[PIPELINE_TYPE_BACK_PROPAGATE].layout, 0, 1, &vulkan.globalDescSet, 0, 0);
        vkCmdPushConstants(vulkan.cmdBuffer, vulkan.pipelines[PIPELINE_TYPE_BACK_PROPAGATE].layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
        vkCmdDispatch(vulkan.cmdBuffer, groupCount, 1, 1);
        AssertSuccess(vkEndCommandBuffer(vulkan.cmdBuffer));
        
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vulkan.cmdBuffer;
        AssertSuccess(vkQueueSubmit(vulkan.computeQueue, 1, &submitInfo, 0));
        AssertSuccess(vkQueueWaitIdle(vulkan.computeQueue));
    }
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
ClearPipelinesAndStorageBuffers()
{
    if(VulkanIsValidHandle(vulkan.device))
    {
        vkDeviceWaitIdle(vulkan.device);
        
        for (u32 i = 0; i < PIPEPLINE_TYPE_COUNT; i++)
        {
            if (VulkanIsValidHandle(vulkan.pipelines[i].handle))
            {
                vkDestroyPipeline(vulkan.device, vulkan.pipelines[i].handle, 0);
                DebugPrint("Cleared Pipeline\n");
            }
            
            if (VulkanIsValidHandle(vulkan.pipelines[i].layout))
            {
                vkDestroyPipelineLayout(vulkan.device, vulkan.pipelines[i].layout, 0);
            }
        }
        
        if(VulkanIsValidHandle(vulkan.globalDescSetLayout))
            vkDestroyDescriptorSetLayout(vulkan.device, vulkan.globalDescSetLayout, 0);
        if(VulkanIsValidHandle(vulkan.globalDescPool))
            vkDestroyDescriptorPool(vulkan.device, vulkan.globalDescPool, 0);
        
        ClearBuffer(vulkan.weightsBuffer);
        ClearBuffer(vulkan.biasesBuffer);
        ClearBuffer(vulkan.valuesBuffer);
        ClearBuffer(vulkan.productsBuffer);
        ClearBuffer(vulkan.errorsBuffer);
        ClearBuffer(vulkan.distPerPixelBuffer);
        ClearBuffer(vulkan.distPerImgBuffer);
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
ClearBuffer(vulkan_buffer &buffer)
{
    if(VulkanIsValidHandle(vulkan.device))
    {
        if(VulkanIsValidHandle(buffer.handle))
        {
            vkDestroyBuffer(vulkan.device, buffer.handle, 0);
            buffer.handle = VK_NULL_HANDLE;
        }
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
        
        ClearPipelinesAndStorageBuffers();
        
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
