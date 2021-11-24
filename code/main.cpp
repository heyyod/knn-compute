#include "hy3d_base.h"
#include <cmath>
#include <iostream>

#include "data.h"
#include "vulkan_platform.cpp"
#include "nearest.cpp"

// NOTE(heyyod): SET TO 1 TO PRINT INFO WHILE THE ALGORITHMS RUN (MUCH SLOWER!)
#define PRINT_ENABLED 0

int main()
{
    image_data trainData = {};
    image_data testData = {};
    if (!ReadData("../data/train-images.idx3-ubyte", "../data/train-labels.idx1-ubyte", trainData))
        return false;
    if (!ReadData("../data/t10k-images.idx3-ubyte", "../data/t10k-labels.idx1-ubyte", testData))
        return false;
    
    bool vulkanEnabled = false;
    if (Vulkan::Initialize())
    {
        if (Vulkan::UploadInputData(testData.pixels, trainData.pixels))
        {
            //free(trainData.pixels);
            //free(testData.pixels);
            vulkanEnabled = true;
            Print("Hardware Acceleration With Vulkan Enabled.\n");
        }
    }
    else
        Print("Hardware Acceleration With Vulkan Not Supported.\n");
    
    u32 nTest = 1000;
    KNearestNeighbour(1, 2.0f, trainData, testData, vulkanEnabled, nTest);
    KNearestNeighbour(3, 2.0f, trainData, testData, vulkanEnabled, nTest);
    NearestCentroid(trainData, testData, nTest);
    
    FreeData(trainData);
    FreeData(testData);
    if (vulkanEnabled)
        Vulkan::Destroy();
    
    Print("\nFinished. Enter any character and press Enter to exit.");
    u8 stop;
    std::cin >> stop;
    return 0;
}
