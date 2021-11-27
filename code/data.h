/* date = November 24th 2021 8:46 pm */

#ifndef DATA_H
#define DATA_H

#define PIXELS_PER_IMAGE 28*28
#define NUM_TRAIN_IMAGES 60000
#define NUM_TEST_IMAGES 10000
#define NUM_CLASSES 10

#define EndianSwap(x) (x = (x>>24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x<<24))
#define DataSize(nImages) (nImages * PIXELS_PER_IMAGE)

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


#endif //DATA_H
