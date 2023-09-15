#if !defined(CREATE_TILEMAP_H)


//#include <time.h>
#if 0
#include "generators.cpp"

u32 *
SortArray(u32 *Array, u32 Size)
{
    u32 *Result = Array; 

    i32 LastElementIndex = 0;
    for(u32 Index = 1;
        Index < Size;
        ++Index)
    {
        u32 Element = Result[Index];
        i32 AddElementIndex = BinarySearch(Result, LastElementIndex, Element);

        for(i32 Index = LastElementIndex + 1;
            Index > AddElementIndex;
            --Index)
        {
            Result[Index] = Result[Index - 1];
        }

        Result[AddElementIndex] = Element;
        LastElementIndex += 1;
    }

    return(Result);
}

u32
ComputeTileColor(i32 RangeX, i32 RangeY, u32 *Pixels, colors *Main)
{
    u32 Result = 0;

    u64 ColorsSum = 0;
    u32 MidColor = 0;
    u32 Index = 0;
    
    for(i32 Y = RangeY;
        Y < RangeY + TILESIDE;
        ++Y)
    {
        for(i32 X = RangeX;
            X < RangeX + TILESIDE;
            ++X)
        {
            Index = Y * 11520 + X;
            i32 ColorIndex =
                BinarySearch(Main->Colors, Main->ColorCount, Pixels[Index]);
            Main->ColorsCounters[ColorIndex]++;
        }
    }

    i32 MostFrequentColorIndex = 0;
    for(i32 ColorIndex = 0;
        ColorIndex < Main->ColorCount;
        ++ColorIndex)
    {
        if(Main->ColorsCounters[ColorIndex] > MostFrequentColorIndex)
        {
            MostFrequentColorIndex = ColorIndex;
        }
        Main->ColorsCounters[ColorIndex] = 0;
    }
    
    Result = Main->Colors[MostFrequentColorIndex];
    
    return(Result);
}

i32
ReadBinaryFile(char *FileName, u16 *Buffer)
{
    i32 Result = 0;
    FILE *fp;

    fp = fopen(FileName, "rb");

    if(fp == NULL)
    {
        printf("Failed to open file %s\n", FileName);
    }
    else
    {
        Result = (i32)fread(Buffer, sizeof(u16), 512, fp);
    }

    return(Result);
}

i32
LoadTileDataAndColors(tile *Tiles, u32 *Colors)
{
    u16 FileContent[512]  {};
    i32 PairsOfBytesReaded = ReadBinaryFile("output.bin", FileContent);
    i32 Result = 0;
    
    i32 TileCount = 0;
    i32 ColorCount = 0;
    for(i32 ItemIndex = 0;
        ItemIndex < PairsOfBytesReaded;
        ItemIndex += 4)
    {
        tile *Tile = &Tiles[TileCount++];
        Tile->Index = FileContent[ItemIndex];
        Tile->Color = ((FileContent[ItemIndex + 1] << 16)
                       | FileContent[ItemIndex + 2]);
        Tile->Z = FileContent[ItemIndex + 3];

        Colors[ColorCount++] = Tile->Color;
    }

    Result = TileCount;

    return(Result);
}

void
WriteTileMapToFile(u32 *Array, i32 Size)
{
    FILE* file = fopen("tilemap.txt", "w");
    FILE* file_py = fopen("tilemap_py.txt", "w");
    
    if((file != NULL) && (file_py != NULL))
    {
        u32 RowCount = 0;
        fprintf(file, "{\n");
        for(i32 i = 0;
            i < Size;
            ++i)
        {
            if(RowCount == 0)
            {
                fprintf(file, "    {0x%x", Array[i]);
                ++RowCount;
            }
            else
            {
                fprintf(file, ", 0x%x ", Array[i]);
                ++RowCount;

                if(RowCount == 384)
                {
                    fprintf(file, "},\n");
                    RowCount = 0;
                }
            }
        }
        fprintf(file, "};");
        fclose(file);

        printf("Array has been written to the file successfully.\n");

        for(i32 i = Size - 1;
            i > 0;
            --i)
        {
            fprintf(file_py, "%u ", Array[i]);
        }
        fclose(file_py);
        printf("Array has been written to the file successfully.\n");
    }
    else
    {
        printf("Unable to open the write file.\n");
    }    
}

union v2
{
    struct
    {
        r32 X, Y;
    };
    r32 E[2];
};

#include "math.h"

inline i32
RoundReal32ToInt32(r32 R32)
{
    i32 Result = (i32)roundf(R32);
    return(Result);
}

inline u32
RoundReal32ToUInt32(r32 R32)
{
    u32 Result = (u32)roundf(R32);
    return(Result);
}

# if 0
int main()
{
    u32 TileMap[138240] = {};
    i32 TileMapSize = ArrayCount(TileMap);

    u32 TilesCounters[256] = {};

    tile MainTiles[256] = {};
    u32 MainColors[256] = {};
    u32 TilesCount = LoadTileDataAndColors(MainTiles, MainColors);
    
#if CHECK_TIME
    clock_t start, end;

    clock_t start_y, end_y;
    clock_t time_end_read;

    r64 time_used;
    r64 time_used_for_iy;
    r64 time_to_read;

    start = clock();
#endif    

    loaded_bitmap BMPFile = LoadBMP("forest_location.bmp");     
    i32 TileMapWidth = BMPFile.Width / TILESIDE; 
    i32 TileMapHeight = BMPFile.Height / TILESIDE; 

#if CHECK_TIME
    time_end_read = clock();
    time_to_read = ((r64)(time_end_read - start)) / CLOCKS_PER_SEC;
    printf("Time To Read File: %.5f seconds\n", time_to_read);
#endif
    
    if(BMPFile.Pixels)
    {
        colors Main = {};
        Main.ColorCount = TilesCount;
        Main.Colors = SortArray(MainColors, Main.ColorCount);
        
        i32 c = 0;
        u32 TileColor = 0;
        u32 Tile = 0;
        u32 Count = 0;
        for(i32 RangeY = 0;
            RangeY < BMPFile.Height;
            RangeY += TILESIDE)
        {
#if CHECK_TIME
            start_y = clock();
#endif
            for(i32 RangeX = 0;
                RangeX < BMPFile.Width;
                RangeX += TILESIDE)
            {
                TileColor = ComputeTileColor(RangeX, RangeY, BMPFile.Pixels, &Main);

                if(TileColor == 0xff00678b)
                {
                    int a = 1;
                }
                for(i32 TileIndex = 0;
                    TileIndex < Main.ColorCount;
                    ++TileIndex)
                {
                    tile *MainTile = &MainTiles[TileIndex];
                    if(TileColor == MainTile->Color)
                    {
                        Tile = (MainTile->Index << 16) | MainTile->Z;
                        c++;
                        break;
                    }
                }
                TileMap[Count++] = Tile;
            }
#if CHECK_TIME
            end_y = clock();
            time_used_for_iy = ((r64)(end_y - start_y)) / CLOCKS_PER_SEC;
            printf("One Y Iteration Time: %.5f seconds\n", time_used_for_iy);
#endif
            i32 a = RandomNumber(1, 100);
            printf("Tiles Set: %d, Random: %d\n", c, a);
        }
        printf("Creating complited!\n");

        // Generate trees
        TreeGenerator(TileMap, 1600, 6, TileMapWidth, TileMapHeight, 28);

        // Generate bushes
        BushGenerator(TileMap, 500, 10, TileMapWidth, TileMapHeight, 30);

        
        WriteTileMapToFile(TileMap, TileMapSize);
    }
    else
    {
        printf("Unable to open the file.\n");
    }

#if CHECK_TIME
    end = clock();
    time_used = ((r64)(end - start)) / CLOCKS_PER_SEC;
    printf("Execution Time: %.5f seconds\n", time_used);
#endif
    
    return(0);
}
#endif
#endif

#define CREATE_TILEMAP_H
#endif
