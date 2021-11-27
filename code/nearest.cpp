#include "data.h"


/* NOTE(heyyod): 
MANHATATTAN DISTANCE -> distP = 1
EUCLIDIAN DISTANCE   -> distP = 2
CHEBYCHEV DISTANCE   -> distP = inf
*/
func f32
KNearestNeighbour(u32 nNeighbours, f32 distP, image_data &trainData, image_data &testData, bool vulkanEnabled, u32 nTest = 0)
{
    std::cout << std::endl << nNeighbours << " Nearest Neighbours Algorithm" << std::endl;
    
    if (nTest == 0)
        nTest = testData.nImages;
    
    Print("Testing " << nTest << " images\n");
    
    u32 *neighbourDists  = (u32 *)malloc(nNeighbours * sizeof(u32));
    u8 *neighbourLabels = (u8 *)malloc(nNeighbours * sizeof (u8));
    u32 furthestNeighbour = 0;
    u32 nSuccess = 0;
    
    TimeStart();
    if (!vulkanEnabled)
    {
        Print("Running on CPU\n");  
        for (u32 iTest = 0; iTest < nTest; iTest++)
        {
            memset(neighbourDists, F32_MAX_HEX, nNeighbours * sizeof(f32));
            for (u32 iTrain = 0; iTrain < trainData.nImages; iTrain++)
            {
                // NOTE(heyyod): calculate the distance using the minkowski distance formula
                u32 dist = 0;
                for (u32 iPixel = 0; iPixel < trainData.pixelsPerImg; iPixel++)
                {
                    f32 pixelDiff = (f32)(trainData.GetPixel(iTrain, iPixel) - testData.GetPixel(iTest, iPixel));
                    dist += (u32)powf(pixelDiff, distP); 
                }
                
                if (dist < neighbourDists[furthestNeighbour])
                {
                    // NOTE(heyyod): Update the neighbour distances and labels
                    neighbourDists[furthestNeighbour] = dist;
                    neighbourLabels[furthestNeighbour] = trainData.labels[iTrain];
                    
                    // NOTE(heyyod): Find the new furthest neighbour
                    for (u32 iNeighbour = 0; iNeighbour < nNeighbours; iNeighbour++)
                    {
                        if (neighbourDists[iNeighbour] > neighbourDists[furthestNeighbour])
                            furthestNeighbour = iNeighbour;
                    }
                }
            }
            
            // NOTE(heyyod): calculate label weights (inverse of distance)
            // so that the nearest neighbours have greater weights
            f32 labelWeights[NUM_CLASSES] = {};
            u32 classifyLabel = 0;
            for (u32 iNeighbour = 0; iNeighbour < nNeighbours; iNeighbour++)
            {
                u32 label = neighbourLabels[iNeighbour];
                if (neighbourDists[iNeighbour] > 0)
                    labelWeights[label] += 1.0f / neighbourDists[iNeighbour];
                else
                    Assert(0);
                
                if (labelWeights[classifyLabel] < labelWeights[label])
                    classifyLabel = label;
            }
            
            // NOTE(heyyod): Print some info
            if (classifyLabel == testData.labels[iTest])
                nSuccess++;
#if PRINT_ENABLED
            system("cls"); // clear console
            std::cout << "~~~~~~ " <<  nNeighbours << " Nearest Neighbours Algorithm ~~~~~~" << std::endl;
            std::cout << "Proccesed " << iTest + 1 << '\\' << nTest << ". ";
            f32 rate = (f32)nSuccess / (f32) (iTest + 1);
            std::cout << "\nSuccess rate: " << rate << std::endl;
            PrintNumber(&testData.pixels[iTest * testData.pixelsPerImg], 28, 28);
            std::cout << "Classified as: " << classifyLabel;
#endif
        }
    }
    else
    {
        Print("Running on GPU\n");
        
        u32 *distPerImage = 0;
        u64 distPerImageBufferSize = 0;
        
        Vulkan::AllocateKnnMemory(&distPerImage, distPerImageBufferSize);
        Vulkan::CreatePipeline(PIPELINE_TYPE_NEAREST_NEIGHBOUR);
        
        // NOTE(heyyod): Shader only supports manhattan distance
        for (u32 iTest = 0; iTest < nTest; iTest++)
        {
            Vulkan::KnnCompute(iTest, (u32)distP);
            
            memset(neighbourDists, F32_MAX_HEX, nNeighbours * sizeof(f32));
            for (u32 iTrain = 0; iTrain < NUM_TRAIN_IMAGES; iTrain++)
            {
                u32 dist = distPerImage[iTrain];
                if (dist < neighbourDists[furthestNeighbour])
                {
                    // NOTE(heyyod): Update the neighbour distances and labels
                    neighbourDists[furthestNeighbour] = dist;
                    neighbourLabels[furthestNeighbour] = trainData.labels[iTrain];
                    
                    // NOTE(heyyod): Find the new furthest neighbour
                    for (u32 iNeighbour = 0; iNeighbour < nNeighbours; iNeighbour++)
                    {
                        if (neighbourDists[iNeighbour] > neighbourDists[furthestNeighbour])
                            furthestNeighbour = iNeighbour;
                    }
                }
            }
            
            // NOTE(heyyod): calculate label weights (inverse of distance)
            // so that the nearest neighbours have greater weights
            f32 labelWeights[NUM_CLASSES] = {};
            u32 classifyLabel = 0;
            for (u32 iNeighbour = 0; iNeighbour < nNeighbours; iNeighbour++)
            {
                u32 label = neighbourLabels[iNeighbour];
                if (neighbourDists[iNeighbour] > 0)
                    labelWeights[label] += 1.0f / neighbourDists[iNeighbour];
                else
                    Assert(0);
                
                if (labelWeights[classifyLabel] < labelWeights[label])
                    classifyLabel = label;
            }
            
            // NOTE(heyyod): Print some info
            if (classifyLabel == testData.labels[iTest])
                nSuccess++;
        }
    }
    TimeEnd();
    
    free(neighbourDists);
    free(neighbourLabels);
    
    f32 rate = (f32)nSuccess / (f32)nTest;
    std::cout << "Success rate: " << rate << std::endl;
    PrintTimeElapsed();
    return rate;
}

func f32
NearestCentroid(image_data &trainData, image_data &testData, u32 nTest = 0)
{
    std::cout << "\nNearest Class Centroid Algorithm" << std::endl;
    Print("Running on CPU\n");
    
    u32 nSuccess = 0;
    if (nTest == 0)
        nTest = testData.nImages;
    Print("Testing " << nTest << " images\n");
    
    // NOTE(heyyod): Find the centers of each class
    // NOTE(heyyod): Allocate one image per class. Pixels are f32 here because we
    // calculate the average value of each pixel from every image
    f32 *centerPixels = (f32 *)malloc(NUM_CLASSES * trainData.pixelsPerImg * sizeof(f32));
    memset(centerPixels, 0, NUM_CLASSES * trainData.pixelsPerImg * sizeof(f32));
    u32 nPerTrainClass[NUM_CLASSES] = {};
    
    TimeStart();
    for (u32 iTrain = 0; iTrain < trainData.nImages; iTrain++)
    {
        u8 label = trainData.labels[iTrain];
        nPerTrainClass[label]++;
    }
    
    for (u32 iTrain = 0; iTrain < trainData.nImages; iTrain++)
    {
        u8 label = trainData.labels[iTrain];
        f32 *img = &centerPixels[label * trainData.pixelsPerImg];
        u32 count = nPerTrainClass[label];
        for (u32 iPixel = 0; iPixel < trainData.pixelsPerImg; iPixel++)
        {
            f32 trainPixel = (f32)trainData.GetPixel(iTrain, iPixel);
            img[iPixel] += (f32)trainPixel / (f32)count;
        }
        
    }
    
    for (u32 iTest = 0; iTest < nTest; iTest++)
    {
        f32 nearestClassDist = F32_MAX_EXP;
        u32 nearestClassLabel = 0;
        
        for (u32 iClass = 0; iClass < NUM_CLASSES; iClass++)
        {
            // NOTE(heyyod): calculate the distance using the minkowski distance formula
            f32 dist = 0.0f;
            f32 *centerImg = &centerPixels[iClass * trainData.pixelsPerImg];
            for (u32 iPixel = 0; iPixel < trainData.pixelsPerImg; iPixel++)
            {
                dist += Abs(centerImg[iPixel] - (f32)testData.GetPixel(iTest, iPixel));
            }
            
            if (dist < nearestClassDist)
            {
                nearestClassDist = dist;
                nearestClassLabel = iClass;
            }
        }
        
        // NOTE(heyyod): Print some info
        if (nearestClassLabel == testData.labels[iTest])
            nSuccess++;
#if PRINT_ENABLED
        system("cls"); // clear console
        std::cout << "~~~~~~ Nearest Class Centroid Algorithm ~~~~~~" << std::endl;
        std::cout << "Proccesed " << iTest + 1 << '\\' << nTest << ". ";
        f32 rate = (f32)nSuccess / (f32) (iTest + 1);
        std::cout << "\nSuccess rate: " << rate << std::endl;
        PrintNumber(&testData.pixels[iTest * testData.pixelsPerImg], 28, 28);
        std::cout << "Classified as: " << nearestClassLabel;
        PrintNumber(&centerPixels[nearestClassLabel * trainData.pixelsPerImg], 28, 28);
        //Sleep(2000);
#endif
    }
    TimeEnd();
    free(centerPixels);
    f32 rate = (f32)nSuccess / (f32)(nTest);
    std::cout << "Success rate: " << rate << std::endl;
    PrintTimeElapsed();
    return rate;
}
