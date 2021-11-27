/* date = November 23rd 2021 10:06 pm */

#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include "data.h"

struct layer
{
    u32 valuesIndex;  // index in the values buffer
    u32 biasesIndex;  // index in the biases buffer
    u32 weightsIndex; // index in the weights buffer
    u32 weightsDim;   // previous layer dim
    u32 dimension;    // number of neurons in this layer
    u32 depth;
};

struct neural_net
{
    u32 nLayers;
    f32 *weights;
    f32 *biases;
    f32 *values;
    layer *layers;
};

#define LayerValues(net, l) (&net.values[net.layers[l].valuesIndex])
#define OutputLayerValues(net) (&net.values[net.layers[net.nLayers-1].valuesIndex])

#endif //NEURAL_NET_H
