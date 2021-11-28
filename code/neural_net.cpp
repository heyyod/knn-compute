#include "neural_net.h"

#include <stdlib.h>
#include <time.h>

#define RandomFloat0to1() (0.01f*(f32)(rand() % 101))

f32 *testProducts;

func bool
CreateNeuralNet(u32* layersDims, u32 nLayers, neural_net &net, image_data trainData, image_data testData)
{
    net.nLayers = nLayers;
    net.layers = (layer *)malloc(nLayers * sizeof(layer));
    if (!Vulkan::AllocateNeuralNetMemory(layersDims, nLayers, &net.weights, &net.biases, &net.values, &net.errors, &testProducts) ||
        !Vulkan::CreatePipeline(PIPELINE_TYPE_FEED_FORWARD) ||
        !Vulkan::CreatePipeline(PIPELINE_TYPE_BACK_PROPAGATE))
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
    // biases/weights/errorsIndex are ignored for layer[0] -> input layer
    
    net.layers[1].valuesIndex = NUM_TRAIN_IMAGES + NUM_TEST_IMAGES;
    net.layers[1].biasesIndex = 0;
    net.layers[1].weightsIndex = 0;
    net.layers[1].errorsIndex= 0;
    
    srand ((u32)time(0));
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
            curr.errorsIndex = prev.errorsIndex + prev.dimension;
            curr.weightsIndex = prev.weightsIndex + prev.dimension * prev.weightsDim;
        }
        
        // NOTE(heyyod): Randomize weights and biases
        for (u32 j = 0; j < curr.dimension; j++)
        {
            net.biases[curr.biasesIndex + j] = RandomFloat0to1();
            for (u32 k = 0; k < curr.weightsDim; k++)
            {
                net.weights[curr.weightsIndex + curr.weightsDim * j + k] = RandomFloat0to1();
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
FeedForward(neural_net &net, u32 iTrain)
{
    for (u32 iLayer = 0; iLayer < net.nLayers - 1; iLayer++)
    {
        u32 inValuesIndex = LayerValuesIndex(net, iLayer);
        if (iLayer == 0)
            inValuesIndex += iTrain * LayerDim(net, 0);
        
        u32 inValuesDim = LayerDim(net, iLayer);
        
        u32 weightsIndex = LayerWeightsIndex(net, iLayer + 1);
        u32 weightsDim = LayerWeightsDim(net, iLayer + 1);
        
        u32 biasesIndex =  LayerBiasesIndex(net, iLayer + 1);
        
        u32 outValuesIndex = LayerValuesIndex(net, iLayer + 1);
        u32 outValuesDim = LayerDim(net, iLayer + 1);
        
        Vulkan::FeedForwardCompute(inValuesIndex, inValuesDim, weightsIndex, weightsDim, biasesIndex, outValuesIndex, outValuesDim);
    }
}

func void
BackPropagate(neural_net &net, u32 trainIndex, f32 learningRate)
{
    for (u32 iLayer = net.nLayers - 1; iLayer > 0; iLayer--)
    {
        u32 prevLayerValuesIndex = LayerValuesIndex(net, iLayer-1);
        if (iLayer == 1)
            prevLayerValuesIndex *= trainIndex * LayerDim(net, iLayer-1);
        
        Vulkan::BackPropagateCompute(LayerValuesIndex(net, iLayer), prevLayerValuesIndex,
                                     LayerErrorsIndex(net, iLayer), LayerDim(net, iLayer),
                                     LayerWeightsIndex(net, iLayer), LayerWeightsDim(net, iLayer),
                                     LayerBiasesIndex(net, iLayer),
                                     LayerErrorsIndex(net, iLayer - 1), LayerDim(net, iLayer - 1),
                                     learningRate, iLayer);
    }
}

func void
TrainNeuralNet(neural_net &net, image_data &trainData)
{
    for (u32 iTrain  = 0; iTrain < NUM_TRAIN_IMAGES; iTrain++)
    {
        FeedForward(net, iTrain);
        
        f32 target[10] = {};
        target[trainData.labels[iTrain]] = 1.0f;
        f32 *output = OutputLayerValues(net);
        f32 *outputErrors = OutputLayerErrors(net);
        for (u32 i = 0; i < 10; i++)
            outputErrors[i] = target[i] - output[i];
        
        // TODO(heyyod): Something is fucked up with the products buffer
        BackPropagate(net, iTrain, 0.1);
    }
}

