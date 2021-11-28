/* date = November 23rd 2021 10:06 pm */

#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include "data.h"

struct layer
{
    u32 valuesIndex;  // index in the values buffer
    u32 biasesIndex;  // index in the biases buffer
    u32 weightsIndex; // index in the weights buffer
    u32 errorsIndex;  // index in the errors buffer
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
    f32 *errors;
    layer *layers;
};

#define LayerValuesIndex(net, l)    (net.layers[l].valuesIndex)
#define LayerBiasesIndex(net, l)    (net.layers[l].biasesIndex)
#define LayerWeightsIndex(net, l)   (net.layers[l].weightsIndex)
#define LayerErrorsIndex(net, l)    (net.layers[l].errorsIndex)
#define LayerWeightsDim(net, l)     (net.layers[l].weightsDim)
#define LayerDim(net, l)            (net.layers[l].dimension)
#define LayerDepth(net, l)          (net.layers[l].depth)
#define LayerValues(net, l)         (&net.values[net.layers[l].valuesIndex])
#define LayerErrors(net, l)         (&net.errors[net.layers[l].errorsIndex])
#define OutputLayerValues(net)      LayerValues(net, net.nLayers - 1)
#define OutputLayerErrors(net)      LayerErrors(net, net.nLayers - 1)
#define OutputLayerDim(net)         LayerDim(net, net.nLayers - 1)

#endif //NEURAL_NET_H
