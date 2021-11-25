/* date = November 23rd 2021 10:06 pm */

#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include "data.h"

struct perceptron
{
};

struct layer
{
    u32 weightsIndex;
    u32 weightsDim;
    union
    {
        u32 biasesIndex;
        u32 valuesIndex;
    };
    u32 dimension;
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

#endif //NEURAL_NET_H
