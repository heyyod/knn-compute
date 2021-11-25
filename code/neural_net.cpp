#include "neural_net.h"

#include <stdlib.h>
#define RandomFloat0to1() (0.01f*(f32)(rand() % 101))

func bool
CreateNeuralNet(u32* layersDims, u32 nLayers, neural_net outNet)
{
    outNet.nLayers = nLayers;
    outNet.layers = (layer *)malloc(nLayers * sizeof(layer));
    if (!Vulkan::AllocateNeuralNetMemory(layersDims, nLayers, &outNet.weights, &outNet.biases, &outNet.values))
        return false;
    
    outNet.layers[0].dimension= layersDims[0];
    outNet.layers[0].depth = 0;
    outNet.layers[1].weightsIndex = 0;
    outNet.layers[1].valuesIndex= 0;
    for (u32 i = 1; i < nLayers; i++)
    {
        layer &prev = outNet.layers[i-1];
        layer &curr = outNet.layers[i];
        
        curr.depth = i;
        curr.dimension = layersDims[i];
        curr.weightsDim = prev.dimension;
        
        if (i > 1)
        {
            curr.weightsIndex = prev.weightsIndex + prev.dimension * prev.weightsDim;
            curr.valuesIndex = prev.valuesIndex + prev.dimension;
        }
        
        // NOTE(heyyod): Randomize weights and biases
        for (u32 j = 0; j < curr.dimension; j++)
        {
            outNet.biases[curr.biasesIndex + j] = RandomFloat0to1();
            for (u32 k = 0; k < curr.weightsDim; k++)
            {
                outNet.weights[curr.weightsIndex + k] = RandomFloat0to1();
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
FeedForward(neural_net &net)
{
    
}

func void
BackPropagate()
{
    
}

func void
Train()
{
}