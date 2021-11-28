#include "hy3d_base.h"
#include <cmath>
#include <iostream>

#include "data.h"
#include "vulkan_platform.cpp"
#include "nearest.cpp"
#include "neural_net.cpp"

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
        vulkanEnabled = true;
        
        Print("---- Hardware Acceleration With Vulkan Enabled ----\n");
        
        neural_net net = {};
        u32 layerDims[] = {PIXELS_PER_IMAGE, 32, 32, NUM_CLASSES};
        if (CreateNeuralNet(layerDims, ArrayCount(layerDims), net, trainData, testData))
        {
            TrainNeuralNet(net, trainData, 0.01);
            TestNeuralNet(net, testData, NUM_TEST_IMAGES);
        }
        
    }
    else
    {
        Print("---- Hardware Acceleration With Vulkan Not Supported ----\n");
    }
    
    Print("\nFinished. Enter any character and press Enter to exit.");
    u8 stop;
    std::cin >> stop;
    return 0;
}
