/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */
#include <windows.h>
#include <stdio.h>
#include <time.h>


#include "main_platform.h"
#include "create_tilemap.h"

inline bit_scan_result
FindLeastSegnificantSetBit(u32 Value)
{
    bit_scan_result Result = {};

#if 1
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(uint32 Test = 0;
        Test < 32;
        ++Test)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif

    return(Result);
}

u32
SafeTruncateUInt64(u64 Value)
{
    // TODO(casey): Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return(Result);
}

void
FreeFileMemory(void *Memory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

debug_read_file_result
ReadEntireFile(char *FileName)
{
    debug_read_file_result Result = {};
    
    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    // NOTE(casey): File read successfully
                    Result.ContentsSize = FileSize32;
                }
                else
                {                    
                    // TODO(casey): Logging
                    FreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
                // TODO(casey): Logging
            }
        }
        else
        {
            // TODO(casey): Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(Result);
}

#pragma pack(push, 1)
struct bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    i32 Width;
    i32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 SizeOfBitmap;
    i32 HorzResolution;
    i32 VertResolution;
    u32 ColorsUsed;
    u32 ColorsImportant;

    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
};
#pragma pack(pop)

loaded_bitmap
LoadBMP(char *FileName)
{
    loaded_bitmap Result = {};
    
    debug_read_file_result ReadResult = ReadEntireFile(FileName);

    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        u32 *Pixels = (u32 *)((u8 *)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;

        Assert(Header->Compression == 3);
        
        // NOTE(casey): If you are using this generically for some reason,
        // please remeber that BMP file CAN GO IN EITHER DIRECTION and
        // the height will be negative for top-down.
        // (Also, there can be compression, etc., etc... DON'T think this
        // is complete BMP loading code because it isn't!!)

        // NOTE(casey): Byte order in memory is determined by the Header itself,
        // so we have to read out the masks and convert the pixels ourselves.
        u32 RedMask = Header->RedMask;
        u32 GreenMask = Header->GreenMask;
        u32 BlueMask = Header->BlueMask;
        u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
        
        bit_scan_result RedShift = FindLeastSegnificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSegnificantSetBit(GreenMask);
        bit_scan_result BlueShift =  FindLeastSegnificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSegnificantSetBit(AlphaMask);

        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(AlphaShift.Found);
        
        u32 *SourceDest = Pixels;
        for(i32 Y = 0;
            Y < Header->Height;
            ++Y)
        {
            for(i32 X = 0;
                X < Header->Width;
                ++X)
            {
                u32 C = *SourceDest;
                *SourceDest++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) |
                                 (((C >> RedShift.Index) & 0xFF) << 16) |
                                 (((C >> GreenShift.Index) & 0xFF) << 8) |
                                 (((C >> BlueShift.Index) & 0xFF) << 0));
            }
        }
    }

    return(Result);
}

i32
BinarySearch(u32 *Array, i32 Count, u32 Element)
{
    i32 Result = 0;
    i32 Low = 0;
    i32 High = Count;

    while(Low <= High)
    {
        i32 Mid = Low + (High - Low) / 2;

        if((Array[Mid] == Element))
        {
            Result = Mid;
            break;
        }
        else if(Array[Mid] < Element)
        {
            Low = Mid + 1;
        }
        else if(Array[Mid] > Element)
        {
            High = Mid - 1;
        }
    }

    if((Array[High] < Element))
    {
        Result = Low;
    }
    
    return(Result);
}

u32 *
SortArray(u32 *Array, u32 Size)
{
    i32 LastElementIndex = 0;
    for(u32 Index = 1;
        Index < Size;
        ++Index)
    {
        u32 Element = Array[Index];
        i32 AddElementIndex = BinarySearch(Array, LastElementIndex, Element);

        for(i32 Index = LastElementIndex + 1;
            Index > AddElementIndex;
            --Index)
        {
            Array[Index] = Array[Index - 1];
        }

        Array[AddElementIndex] = Element;
        LastElementIndex += 1;
    }

    return(Array);
}

u32
FindTileColor(u32 MidColor, arguments *Main)
{
    u32 Result = 0;

    u32 TileColorIndex = (u32)BinarySearch(Main->Colors, Main->ColorCount, MidColor);
    u32 MainColorsDiff = (Main->Colors[TileColorIndex + 1] - Main->Colors[TileColorIndex]);

    if((MidColor >= Main->Colors[TileColorIndex]) &&
       (MidColor <= (Main->Colors[TileColorIndex] + MainColorsDiff)))
    {
        Result = Main->Colors[TileColorIndex];
    }
    else
    {
        Result = Main->Colors[TileColorIndex + 1];
    }
    
    return(Result);
}

u32
ComputeTileColor(i32 RangeX, i32 RangeY, u32 *Pixels, arguments *Main)
{
    u32 Result = 0;

    u64 ColorsSum = 0;
    u32 MidColor = 0;
    u32 Index = 0;

    for(i32 Y = RangeY;
        Y < RangeY + Main->TileSide;
        ++Y)
    {
        for(i32 X = RangeX;
            X < RangeX + Main->TileSide;
            ++X)
        {
            Index = Y * 11520 + X;
            ColorsSum += Pixels[Index];
        }
    }
    
    MidColor = (u32)(ColorsSum / (Main->TileSide * Main->TileSide));
    
    Result = FindTileColor(MidColor, Main);
    
    return(Result);
}

int main()
{

    clock_t start, end;

    clock_t start_y, end_y;
    clock_t time_end_read;

    r64 time_used;
    r64 time_used_for_iy;
    r64 time_to_read;
    
    i32 TileSide = 30;

    u32 TileMap[138240] = {};

    u32 MainColorsOriginal[14] =
        {
            0xff3aa200, 0xff0d760c, 0xff00678b, 0xffffd184, 0xffab8e0f,
            0xff6c6b86, 0xff5858a5, 0xff000000, 0xff000001, 0xff000002,
            0xff000003, 0xff000004, 0xff000005, 0xff000006
        };

    u16 MainTilesIndexes[14] =
        {
            0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
            0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e
        };

    u16 MainTilesZCoord[14] =
        {
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0002, 0x0002, 0x0002, 0x0001, 0x0003, 0x0003, 0x0002 
        };

    u32 MainColors[14] =
        {
            0xff3aa200, 0xff0d760c, 0xff00678b, 0xffffd184, 0xffab8e0f,
            0xff6c6b86, 0xff5858a5, 0xff000000, 0xff000001, 0xff000002,
            0xff000003, 0xff000004, 0xff000005, 0xff000006
        };

    start = clock();
    
    loaded_bitmap BMPFile = LoadBMP("forest_location.bmp");     

    time_end_read = clock();
    time_to_read = ((r64)(time_end_read - start)) / CLOCKS_PER_SEC;
    
    printf("Time To Read File: %.5f seconds\n", time_to_read);

#if 1
    if(BMPFile.Width)
    {
        arguments Main = {};
        Main.ColorCount = ArrayCount(MainColors);
        Main.Colors = SortArray(MainColors, Main.ColorCount);
        Main.TileSide = TileSide;
    
        u32 TileColor = 0;
        u32 Tile = 0;
        u32 Count = 0;

        for(i32 RangeY = BMPFile.Height - TileSide;
            RangeY >= 0;
            RangeY -= TileSide)
        {
            start_y = clock();
            for(i32 RangeX = 0;
                RangeX < BMPFile.Width;
                RangeX += TileSide)
            {
                TileColor = ComputeTileColor(RangeX, RangeY, BMPFile.Pixels, &Main);
                for(i32 TileIndex = 0;
                    TileIndex < ArrayCount(MainColors);
                    ++TileIndex)
                {
                    if(TileColor == MainColorsOriginal[TileIndex])
                    {
                        Tile = (MainTilesIndexes[TileIndex] << 16) | MainTilesZCoord[TileIndex];
                        TileMap[Count++] = Tile;
                        break;
                    }
                }
            }
            end_y = clock();
            time_used_for_iy = ((r64)(end_y - start_y)) / CLOCKS_PER_SEC;
            printf("One Y Iteration Time: %.5f seconds\n", time_used_for_iy);
        }

        printf("Comleted");
        
        FILE* file = fopen("array.txt", "w"); // "wb" for binary mode

        i32 Size = ArrayCount(TileMap);
    
        if(file != NULL)
        {
            u32 RowCount = 0;
            fprintf(file, "{");
            for(i32 i = 0;
                i < Size;
                ++i)
            {
                fprintf(file, "0x%x, ", TileMap[i]);
                ++RowCount;
                if(RowCount == 384)
                {
                    fprintf(file, "\n");
                    RowCount = 0;
                }
            }
            fprintf(file, "}");

            fclose(file);
            printf("Array has been written to the file successfully.\n");
        }
        else
        {
            printf("Unable to open the write file.\n");
        }    
    }
    else
    {
        printf("Unable to open the file.\n");
    }
#endif

    end = clock();

    time_used = ((r64)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Execution Time: %.5f seconds\n", time_used);

    return(0);
}
