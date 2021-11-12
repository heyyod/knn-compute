#include "hy3d_base.h"
#include <cmath>
#include <iostream>

#include <chrono>
#include <thread>
global_var std::chrono::steady_clock::time_point timeStart;
global_var std::chrono::steady_clock::time_point timeEnd;
global_var std::chrono::duration<f32> duration;
global_var f32 elapsedTime;
#define Now() std::chrono::steady_clock::now()
#define TimeStart() timeStart = Now()
#define TimeEnd() \
timeEnd = Now(); \
duration = timeEnd - timeStart; \
elapsedTime = duration.count()
#define PrintTimeElapsed() \
Print("Time Elapsed: " << elapsedTime << "s" << std::endl)
#define SleepFor(x) std::this_thread::sleep_for(std::chrono::milliseconds(x));


#define PIXELS_PER_IMAGE 28*28
#define TRAIN_NUM_IMAGES 60000
#define TEST_NUM_IMAGES 10000
#define NUM_CLASSES 10

#define EndianSwap(x) (x = (x>>24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x<<24))
#define DataSize(nImages) (nImages * PIXELS_PER_IMAGE)

// NOTE(heyyod): SET TO 1 TO PRINT INFO WHILE THE ALGORITHMS RUN (MUCH SLOWER!)
#define PRINT_ENABLED 0

#include "vulkan_platform.cpp"

/* 
 The training set contains 60000 examples, and the test set 10000 examples.

The first 5000 examples of the test set are taken from the original NIST training set. 
The last 5000 are taken from the original NIST test set. The first 5000 are cleaner and 
easier than the last 5000.

TRAINING SET LABEL FILE (train-labels-idx1-ubyte):
[offset] [type]          [value]          [description]
0000     32 bit integer  0x00000801(2049) magic number (MSB first)
0004     32 bit integer  60000            number of items
0008     unsigned byte   ??               label
0009     unsigned byte   ??               label
........
xxxx     unsigned byte   ??               label

The labels values are 0 to 9.
TRAINING SET IMAGE FILE (train-images-idx3-ubyte):
[offset] [type]          [value]          [description]
0000     32 bit integer  0x00000803(2051) magic number
0004     32 bit integer  60000            number of images
0008     32 bit integer  28               number of rows
0012     32 bit integer  28               number of columns
0016     unsigned byte   ??               pixel
0017     unsigned byte   ??               pixel
........
xxxx     unsigned byte   ??               pixel

Pixels are organized row-wise. Pixel values are 0 to 255. 0 means background (white), 255 means foreground (black). 
*/

struct image_data
{
    u32 nImages;
    u32 pixelsPerImg; // per image
    u8 *pixels;
    u8 *labels;
    
    inline u8 GetPixel(u32 imgIndex, u32 pxlIndex)
    {
        return pixels[imgIndex * pixelsPerImg + pxlIndex];
    }
};

func bool
ReadData(char *imagesFilepath, char *labelsFilepath, image_data &dataOut)
{
    FILE *imagesFile = fopen(imagesFilepath, "rb");
    FILE *labelsFile = fopen(labelsFilepath, "rb");
    if (!imagesFile || !labelsFile)
    {
        return false;
    }
    
    // NOTE(heyyod): Read Pixels
    u32 magicNumber;
    u32 imagesCount;
    u32 rowCount;
    u32 columnCount;
    fread(&magicNumber, sizeof(magicNumber), 1, imagesFile);
    fread(&imagesCount, sizeof(imagesCount), 1, imagesFile);
    fread(&rowCount, sizeof(rowCount), 1, imagesFile);
    fread(&columnCount, sizeof(columnCount), 1, imagesFile);
    EndianSwap(magicNumber);
    EndianSwap(imagesCount);
    EndianSwap(rowCount);
    EndianSwap(columnCount);
    
    dataOut.nImages = imagesCount;
    dataOut.pixelsPerImg= rowCount * columnCount;
    
    u32 nPixels = imagesCount * rowCount * columnCount;
    if (!dataOut.pixels)
        dataOut.pixels = (u8 *)malloc(nPixels * sizeof(u8));
    fread(dataOut.pixels, sizeof(u8), nPixels, imagesFile);
    
    // NOTE(heyyod): Read Labels
    u32 labelsCount;
    fread(&magicNumber, sizeof(magicNumber), 1, labelsFile);
    fread(&labelsCount, sizeof(labelsCount), 1, labelsFile);
    EndianSwap(magicNumber);
    EndianSwap(labelsCount);
    
    if (!dataOut.labels)
        dataOut.labels = (u8 *)malloc(labelsCount * sizeof(u8));
    fread(dataOut.labels, sizeof(u8), labelsCount, labelsFile);
    
    fclose(imagesFile);
    fclose(labelsFile);
    return true;
}

func void
FreeData(image_data &data)
{
    free(data.pixels);
    free(data.labels);
}

func void
PrintNumber(u8 *pixels, u32 rows, u32 cols)
{
    for (u32 r = 0; r < rows; r++)
    {
        for (u32 c = 0; c < cols; c++)
        {
            if (pixels[r * cols + c] > 210)
                std::cout << '#';
            else
                std::cout << ' ';
        }
        std::cout << std::endl;
    }
}

func void
PrintNumber(f32 *pixels, u32 rows, u32 cols)
{
    for (u32 r = 0; r < rows; r++)
    {
        for (u32 c = 0; c < cols; c++)
        {
            if (pixels[r * cols + c] > 120.0f)
                std::cout << '#';
            else
                std::cout << ' ';
        }
        std::cout << std::endl;
    }
}

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
        
        Vulkan::CreatePipeline(PIPELINE_NEAREST_NEIGHBOUR, &distPerImage, distPerImageBufferSize);
        
        // NOTE(heyyod): Shader only supports manhattan distance
        for (u32 iTest = 0; iTest < nTest; iTest++)
        {
            Vulkan::Compute(iTest, (u32)distP);
            
            memset(neighbourDists, F32_MAX_HEX, nNeighbours * sizeof(f32));
            for (u32 iTrain = 0; iTrain < TRAIN_NUM_IMAGES; iTrain++)
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
    std::cout << "Running: Nearest Class Centroid Algorithm" << std::endl;
    // NOTE(heyyod): Find the centers of each class
    // NOTE(heyyod): Allocate one image per class. Pixels are f32 here because we
    // calculate the average value of each pixel from every image
    f32 *centerPixels = (f32 *)malloc(NUM_CLASSES * trainData.pixelsPerImg * sizeof(f32));
    memset(centerPixels, 0, NUM_CLASSES * trainData.pixelsPerImg * sizeof(f32));
    u32 nPerTrainClass[NUM_CLASSES] = {};
    
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
    
    u32 nSuccess = 0;
    if (nTest == 0)
        nTest = testData.nImages;
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
    
    free(centerPixels);
    f32 rate = (f32)nSuccess / (f32)(nTest);
    return rate;
}

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
    
    u32 nTest = 20;
    KNearestNeighbour(1, 2.0f, trainData, testData, vulkanEnabled, nTest);
    KNearestNeighbour(1, 2.0f, trainData, testData, 0, nTest);
    //KNearestNeighbour(3, 2.0f, trainData, testData, vulkanEnabled, nTest);
    //KNearestNeighbour(3, 2.0f, trainDataCPU, testDataCPU, 0, nTest);
    
    /* 
        TimeStart();
        NearestCentroid(trainData, testData);
        PrintTimeElapsed();
     */
    
    if (!vulkanEnabled)
    {
        FreeData(trainData);
        FreeData(testData);
    }
    
    return 0;
}
