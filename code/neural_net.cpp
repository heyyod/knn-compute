#include "neural_net.h"

#include <stdlib.h>
#define RandomFloat0to1() (0.01f*(f32)(rand() % 101))

f32 *testWV;

func bool
CreateNeuralNet(u32* layersDims, u32 nLayers, neural_net &net, image_data trainData, image_data testData)
{
    net.nLayers = nLayers;
    net.layers = (layer *)malloc(nLayers * sizeof(layer));
    if (!Vulkan::AllocateNeuralNetMemory(layersDims, nLayers, &net.weights, &net.biases, &net.values, &testWV) ||
        !Vulkan::CreatePipeline(PIPELINE_TYPE_FEED_FORWARD))
        return false;
    
    // NOTE(heyyod): Normalize the input data
    for (u32 i = 0; i < NUM_TRAIN_IMAGES; i++)
    {
        net.values[i] = (f32)trainData.pixels[i] / 255.0f;
    }
    for (u32 i = NUM_TRAIN_IMAGES; i < NUM_TRAIN_IMAGES + NUM_TEST_IMAGES; i++)
    {
        net.values[i] = (f32)testData.pixels[i] / 255.0f;
    }
    
    net.layers[0].dimension= layersDims[0];
    net.layers[0].depth = 0;
    net.layers[0].valuesIndex = 0;
    // biasesIndex & weightsIndex are ignored for layer[0] -> input layer
    
    net.layers[1].valuesIndex = NUM_TRAIN_IMAGES + NUM_TEST_IMAGES;
    net.layers[1].biasesIndex = 0;
    net.layers[1].weightsIndex = 0;
    
    for (u32 i = 1; i < nLayers; i++)
    {
        layer &prev = net.layers[i-1];
        layer &curr = net.layers[i];
        
        curr.depth = i;
        curr.dimension = layersDims[i];
        curr.weightsDim = prev.dimension;
        
        if (i > 1)
        {
            curr.valuesIndex = prev.valuesIndex + prev.dimension;
            curr.biasesIndex = prev.biasesIndex + prev.dimension;
            curr.weightsIndex = prev.weightsIndex + prev.dimension * prev.weightsDim;
        }
        
        // NOTE(heyyod): Randomize weights and biases
        for (u32 j = 0; j < curr.dimension; j++)
        {
            net.biases[curr.biasesIndex + j] = RandomFloat0to1();
            for (u32 k = 0; k < curr.weightsDim; k++)
            {
                net.weights[curr.weightsIndex + k] = RandomFloat0to1();
            }
        }
    }
    return true;
}

// TODO(heyyod): Save and Load Neural Net functions

func void
FreeNeuralNet(neural_net &net)
{
    free(net.layers);
}

func void
BackPropagate()
{
    
}

func void
TrainNeuralNet(neural_net &net)
{
    for (u32 iTrain  = 0; iTrain < NUM_TRAIN_IMAGES; iTrain++)
    {
        for (u32 iLayer = 0; iLayer < net.nLayers - 1; iLayer++)
        {
            u32 inValuesIndex = net.layers[iLayer].valuesIndex;
            if (iLayer == 0)
            {
                inValuesIndex+= iTrain * PIXELS_PER_IMAGE;
            }
            u32 inValuesDim = net.layers[iLayer].dimension;
            
            u32 weightsIndex = net.layers[iLayer + 1].weightsIndex;
            u32 weightsDim = net.layers[iLayer + 1].weightsDim;
            
            u32 biasesIndex = net.layers[iLayer + 1].biasesIndex;
            
            u32 outValuesIndex = net.layers[iLayer + 1].valuesIndex;
            u32 outValuesDim = net.layers[iLayer + 1].dimension;
            
            Vulkan::FeedForwardCompute(inValuesIndex, inValuesDim, weightsIndex, weightsDim, biasesIndex, outValuesIndex, outValuesDim);
        }
    }
}

