/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */
//#include <time.h>
#if 0
#include "generators.cpp"

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
    b32 Found = false;

    
    while(Low <= High)
    {
        i32 Mid = Low + (High - Low) / 2;

        if((Array[Mid] == Element))
        {
            Result = Mid;
            Found = true;
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

    if((Array[High] < Element) && (High >= 0) && !Found)
    {
        Result = Low;
    }
    
    return Result;
}

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

/*
u32
FindTileColor(u32 MidColor, colors *Main)
{
    u32 Result = 0;

    i32 TileColorIndex = (u32)BinarySearch(Main->Colors, Main->ColorCount, MidColor);
    u32 MainColorsDiff = (Main->Colors[TileColorIndex + 1] - Main->Colors[TileColorIndex]);

    if(MidColor == Main->Colors[TileColorIndex])
    {
        Result = Main->Colors[TileColorIndex];
    }
    else if(TileColorIndex == (Main->ColorCount - 1))
    {
        Result = Main->Colors[TileColorIndex];
    }
    else if((MidColor > Main->Colors[TileColorIndex]) &&
            (MidColor < (Main->Colors[TileColorIndex] + MainColorsDiff)))
    {
        Result = Main->Colors[TileColorIndex];
    }
    else
    {
        Result = Main->Colors[TileColorIndex + 1];
    }
    
    return(Result);
}
*/
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

internal void
DrawRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax, r32 R, r32 G, r32 B)
{    
    i32 MinX = RoundReal32ToInt32(vMin.X);
    i32 MinY = RoundReal32ToInt32(vMin.Y);
    i32 MaxX = RoundReal32ToInt32(vMax.X);
    i32 MaxY = RoundReal32ToInt32(vMax.Y);

    if(MinX < 0)
    {
        MinX = 0;
    }

    if(MinY < 0)
    {
        MinY = 0;
    }

    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    u32 Color = ((RoundReal32ToUInt32(R * 255.0f) << 16) |
                    (RoundReal32ToUInt32(G * 255.0f) << 8) |
                    (RoundReal32ToUInt32(B * 255.0f) << 0));

    u8 *Row = ((u8 *)Buffer->Memory +
                  MinX*Buffer->BytesPerPixel +
                  MinY*Buffer->Pitch);
    for(int Y = MinY;
        Y < MaxY;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(int X = MinX;
            X < MaxX;
            ++X)
        {            
            *Pixel++ = Color;
        }
        
        Row += Buffer->Pitch;
    }
}

internal void
GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {

        //TODO(casey): This may be more appropriate to do in platform layer
        Memory->IsInitialized = true;
    }
    
    for(int ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        //NOTE(casey): Use digital movement tuning
        if(Controller->MoveLeft.EndedDown)
        {
            OutputDebugStringA("Left\n");
        }

        if(Controller->MoveRight.EndedDown)
        {
            OutputDebugStringA("Right\n");
        }
    }    

    v2 vMin = {0.0f, 0.0f};
    v2 vMax = {100.0f, 100.0f};
    r32 R = 0.25f;
    r32 G = 0.25f;
    r32 B = 0.25f;
    DrawRectangle(Buffer, vMin, vMax, R, G, B);
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


#include "create_tilemap_.h"
#include "create_tilemap_render_group.cpp"
#include "create_tilemap_world.cpp"
#include "create_tilemap_random.h"

#if 1
// TODO(paul): Learn how to do asset streaming and remove this
#include <stdio.h>
#endif

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 SizeOfBitmap;
    int32 HorzResolution;
    int32 VertResolution;
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};
#pragma pack(pop)

inline v2
TopDownAlign(loaded_bitmap *Bitmap, v2 Align)
{
    Align.y = (real32)(Bitmap->Height - 1) - Align.y;

    Align.x = SafeRatio0(Align.x, (real32)Bitmap->Width);
    Align.y = SafeRatio0(Align.y, (real32)Bitmap->Height);
    
    return(Align);
}

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName,
             int32 AlignX, int32 TopDownAlignY)
{
    loaded_bitmap Result = {};
    
    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);    
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.AlignPercentage = TopDownAlign(&Result, V2i(AlignX, TopDownAlignY));
        Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);
        
        Assert(Result.Height >= 0);
        Assert(Header->Compression == 3);

        // NOTE(casey): If you are using this generically for some reason,
        // please remember that BMP files CAN GO IN EITHER DIRECTION and
        // the height will be negative for top-down.
        // (Also, there can be compression, etc., etc... DON'T think this
        // is complete BMP loading code because it isn't!!)

        // NOTE(casey): Byte order in memory is determined by the Header itself,
        // so we have to read out the masks and convert the pixels ourselves.
        uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);        
        
        bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
        
        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);

        int32 RedShiftDown = (int32)RedScan.Index;
        int32 GreenShiftDown = (int32)GreenScan.Index;
        int32 BlueShiftDown = (int32)BlueScan.Index;
        int32 AlphaShiftDown = (int32)AlphaScan.Index;
        
        uint32 *SourceDest = Pixels;
        for(int32 Y = 0;
            Y < Header->Height;
            ++Y)
        {
            for(int32 X = 0;
                X < Header->Width;
                ++X)
            {
                uint32 C = *SourceDest;

                v4 Texel  =
                    {
                        (real32)((C & RedMask) >> RedShiftDown),
                        (real32)((C & GreenMask) >> GreenShiftDown),
                        (real32)((C & BlueMask) >> BlueShiftDown),
                        (real32)((C & AlphaMask) >> AlphaShiftDown)
                    };

                Texel = SRGB255ToLinear1(Texel);

#if 1
                Texel.rgb *= Texel.a;
#endif
                Texel = Linear1ToSRGB255(Texel);
                
                *SourceDest++ = (((uint32)(Texel.a + 0.5f) << 24) |
                                 ((uint32)(Texel.r + 0.5f) << 16) |
                                 ((uint32)(Texel.g + 0.5f) << 8) |
                                 ((uint32)(Texel.b + 0.5f) << 0));
            }
        }
    }

    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
#if 0
    Result.Memory = (uint8 *)Result.Memory + Result.Pitch*(Result.Height - 1);
    Result.Pitch = -Result.Pitch;
#endif
    
    return(Result);
}

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName)
{
    loaded_bitmap Result = DEBUGLoadBMP(Thread, ReadEntireFile, FileName, 0, 0);
    Result.AlignPercentage = V2(0.5f, 0.5f);

    return(Result);
}


internal void
FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
{
    // TODO(casey): Decide what our pushbuffer size is!
    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);

    loaded_bitmap *Buffer = &GroundBuffer->Bitmap;
    Buffer->AlignPercentage = V2(0.5f, 0.5f);
    Buffer->WidthOverHeight = 1.0f; 

    real32 Width = GameState->World->ChunkDimInMeters.x;
    real32 Height = GameState->World->ChunkDimInMeters.y;
    Assert(Width == Height);
    v2 HalfDim = 0.5f*V2(Width, Height);

    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4));
    Ortographic(RenderGroup, Buffer->Width, Buffer->Height, Buffer->Width / Width);
    Clear(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f));

    GroundBuffer->P = *ChunkP;
    HalfDim = 2.0f*HalfDim;

    TiledRenderGroupToOutput(TranState->RenderQueue, RenderGroup, Buffer);
    EndTemporaryMemory(GroundMemory);
}

internal void
ClearBitmap(loaded_bitmap *Bitmap)
{
    if(Bitmap->Memory)
    {
        int32 TotalBitmapSize = Bitmap->Width*Bitmap->Height*BITMAP_BYTES_PER_PIXEL;
        ZeroSize(TotalBitmapSize, Bitmap->Memory);
    }
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *Arena, int32 Width, int32 Height, bool32 ClearToZero = true)
{
    loaded_bitmap Result = {};

    Result.Width = Width;
    Result.Height = Height;
    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
    int32 TotalBitmapSize = Width*Height*BITMAP_BYTES_PER_PIXEL;
    Result.Memory = PushSize(Arena, TotalBitmapSize);
    if(ClearToZero)
    {
        ClearBitmap(&Result);
    }

    return(Result);
}

inline world_position
ChunkPositionFromTilePosition(world *World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ,
                              v3 AdditionalOffset = V3(0, 0, 0))
{
    world_position BasePos = {};

    real32 TileSideInMeters = 1.4f;
    real32 TileDepthInMeters = 3.0f;
    
    v3 TileDim = V3(TileSideInMeters, TileSideInMeters, TileDepthInMeters);
    v3 Offset = Hadamard(TileDim, V3((real32)AbsTileX, (real32)AbsTileY, (real32)AbsTileZ));
    world_position Result = MapIntoChunkSpace(World, BasePos, AdditionalOffset + Offset);
    
    Assert(IsCanonical(World, Result.Offset_));
    
    return(Result);
}

#if CREATE_TILEMAP_INTERNAL
game_memory *DebugGlobalMemory;
#endif

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{    
    PlatformAddEntry = Memory->PlatformAddEntry;
    PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;

#if CREATE_TILEMAP_INTERNAL
    DebugGlobalMemory = Memory;
#endif

    BEGIN_TIMED_BLOCK(GameUpdateAndRender)
                        
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));

    uint32 GroundBufferWidth = 256; 
    uint32 GroundBufferHeight = 256;

    real32 PixelsToMeters = 1.0f / 42.0f;

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        uint32 TilesPerWidth = 33;
        uint32 TilesPerHeight = 19;

        GameState->TypicalFloorHeight = 3.0f;

        v3 WorldChunkDimInMeters = {PixelsToMeters*(real32)GroundBufferWidth,
                                    PixelsToMeters*(real32)GroundBufferHeight,
                                    GameState->TypicalFloorHeight};

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));


        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        InitializeWorld(World, WorldChunkDimInMeters);

        real32 TileSideInMeters = 1.4f;
        real32 TileDepthInMeters = GameState->TypicalFloorHeight;
        
        random_series Series = RandomSeed(1234);
        
        uint32 ScreenBaseX = 0;
        uint32 ScreenBaseY = 0;
        uint32 ScreenBaseZ = 0;
        uint32 ScreenX = ScreenBaseX;
        uint32 ScreenY = ScreenBaseY;
        uint32 AbsTileZ = ScreenBaseZ;
        for(uint32 ScreenIndex = 0;
            ScreenIndex < 48;
            ++ScreenIndex)
        {
            for(uint32 TileY = 0;
                TileY < TilesPerHeight;
                ++TileY)
            {
                for(uint32 TileX = 0;
                    TileX < TilesPerWidth;
                    ++TileX)
                {
                    uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;
                    
                    bool32 ShouldBeDoor = false;
                    if((TileX == 0) && (TileY != (TilesPerHeight/2)))
                    {
                        ShouldBeDoor = true;
                    }

                    if((TileX == (TilesPerWidth - 1)) && (TileY != (TilesPerHeight/2)))
                    {
                        ShouldBeDoor = true;
                    }
                    
                    if((TileY == 0) && (TileX != (TilesPerWidth/2)))
                    {
                        ShouldBeDoor = true;
                    }

                    if((TileY == (TilesPerHeight - 1)) && (TileX != (TilesPerWidth/2)))
                    {
                        ShouldBeDoor = true;
                    }

                    if(ShouldBeDoor)
                    {
//                        AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
                    }
                }
            }

            if(ScreenX < 7)
            {
                ++ScreenX;
            }
            else
            {
                ScreenX = 0;
            }

            if((ScreenY < 5) && (ScreenX == 0))
            {
                ++ScreenY;
            }
        }

#if 0
        while(GameState->LowEntityCount < (ArrayCount(GameState->LowEntities) - 16))
        {
            uint32 Coordinate = 1024 + GameState->LowEntityCount;
            AddWall(GameState, Coordinate, Coordinate, Coordinate);
        }
#endif
        
        world_position NewCameraP = {};
        uint32 CameraTileX = ScreenBaseX*TilesPerWidth + TilesPerWidth/2;
        uint32 CameraTileY = ScreenBaseY*TilesPerHeight + TilesPerHeight/2;
        uint32 CameraTileZ = ScreenBaseZ;
        NewCameraP = ChunkPositionFromTilePosition(GameState->World,
                                                   CameraTileX,
                                                   CameraTileY,
                                                   CameraTileZ);
        GameState->CameraP = NewCameraP;

        Memory->IsInitialized = true;
    }

    // NOTE(casey): Transient initialization
    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);    
    transient_state *TranState = (transient_state *)Memory->TransientStorage;
    if(!TranState->IsInitialized)
    {
        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));

        TranState->RenderQueue = Memory->HighPriorityQueue;
#if 0
        // TODO(casey): Pick a real number here!
        TranState->GroundBufferCount = 128;
        TranState->GroundBuffers = PushArray(&TranState->TranArena, TranState->GroundBufferCount, ground_buffer);
        for(uint32 GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
            GroundBuffer->P = NullPosition();
        }

        GameState->TestDiffuse = MakeEmptyBitmap(&TranState->TranArena, 256, 256, false);
        DrawRectangle(&GameState->TestDiffuse, V2(0, 0),
                      V2i(GameState->TestDiffuse.Width, GameState->TestDiffuse.Height),
                      V4(0.5f, 0.5f, 0.5f, 1.0f));
        GameState->TestNormal =
            MakeEmptyBitmap(&TranState->TranArena,GameState->TestDiffuse.Width,
                            GameState->TestDiffuse.Height, false);
        MakeSphereNormalMap(&GameState->TestNormal, 0.0f);
        MakeSphereDiffuseMap(&GameState->TestDiffuse);
        
        TranState->EnvMapWidth = 512;
        TranState->EnvMapHeight = 256;
        for(uint32 MapIndex = 0;
            MapIndex < ArrayCount(TranState->EnvMaps);
            ++MapIndex)
        {
            environment_map *Map = TranState->EnvMaps + MapIndex;
            uint32 Width = TranState->EnvMapWidth;
            uint32 Height = TranState->EnvMapHeight;
            for(uint32 LODIndex = 0;
                LODIndex < ArrayCount(Map->LOD);
                ++LODIndex)
            {
                Map->LOD[LODIndex] = MakeEmptyBitmap(&TranState->TranArena, Width, Height, false);
                Width >>= 1;
                Height >>= 1;
            }
        }
#endif
        
        TranState->IsInitialized = true;
    }

#if 0
    if(Input->ExecutableReloaded)
    {
        for(uint32 GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->P = NullPosition();            
        }        
    }
#endif    

    world *World = GameState->World;

    //
    // NOTE(casey): 
    //
    for(int ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);

        if(Controller->IsAnalog)
        {
            // NOTE(casey): Use analog movement tuning
            //ConHero->ddP = V2(Controller->StickAverageX, Controller->StickAverageY);
        }
        else
        {
            // NOTE(casey): Use digital movement tuning
            if(Controller->MoveUp.EndedDown)
            {
                GameState->CameraP.ChunkY += 1;
            }
            if(Controller->MoveDown.EndedDown)
            {
                GameState->CameraP.ChunkY += -1;
            }
            if(Controller->MoveLeft.EndedDown)
            {
                GameState->CameraP.ChunkX = -1;
            }
            if(Controller->MoveRight.EndedDown)
            {
                GameState->CameraP.ChunkX = 1;
            }
        }
    }
    
    //
    // NOTE(casey): Render
    //
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    
    loaded_bitmap DrawBuffer_ = {};
    loaded_bitmap *DrawBuffer = &DrawBuffer_;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
    DrawBuffer->Memory = Buffer->Memory;

    // TODO(casey): Decide what our pushbuffer size is!
    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4));
    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;
    Prespective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, 0.6f, 9.0f);
    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

    v2 ScreenCenter = {0.5f*(real32)DrawBuffer->Width,
                       0.5f*(real32)DrawBuffer->Height};

    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
    rectangle3 CameraBoundsInMeters = RectMinMax(V3(ScreenBounds.Min, 0.0f),
                                                 V3(ScreenBounds.Max, 0.0f));
    CameraBoundsInMeters.Min.z = -3.0f*GameState->TypicalFloorHeight;
    CameraBoundsInMeters.Max.z =  1.0f*GameState->TypicalFloorHeight;

#if 0

    // NOTE(casey): Ground chunk rendering
    for(uint32 GroundBufferIndex = 0;
        GroundBufferIndex < TranState->GroundBufferCount;
        ++GroundBufferIndex)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
        if(IsValid(GroundBuffer->P))
        {
            loaded_bitmap *Bitmap = &GroundBuffer->Bitmap;
            v3 Delta = Subtract(GameState->World, &GroundBuffer->P, &GameState->CameraP);        
            if((Delta.z >= -1.0f) && (Delta.z < 1.0f))
            {
                real32 GroundSideInMeters = World->ChunkDimInMeters.x;
//                PushBitmap(RenderGroup, Bitmap, GroundSideInMeters, Delta);

                PushRectOutline(RenderGroup, Delta, V2(GroundSideInMeters, GroundSideInMeters),
                                V4(1.0f, 1.0f, 0.0f, 1.0f));
            }
        }
    }
#endif

#if 1

    // TODO(paul): Update only updatable ground chunks
    // NOTE(casey): Ground chunk updating
    {
        world_position MinChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMeters));
        world_position MaxChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMeters));

        for(int32 ChunkZ = MinChunkP.ChunkZ;
            ChunkZ <= MaxChunkP.ChunkZ;
            ++ChunkZ)
        {
            for(int32 ChunkY = MinChunkP.ChunkY;
                ChunkY <= MaxChunkP.ChunkY;
                ++ChunkY)
            {
                for(int32 ChunkX = MinChunkP.ChunkX;
                    ChunkX <= MaxChunkP.ChunkX;
                    ++ChunkX)
                {
                    world_position ChunkP = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
                    v3 Delta = Subtract(GameState->World, &ChunkP, &GameState->CameraP);        
                    real32 GroundSideInMeters = World->ChunkDimInMeters.x;
                    PushRectOutline(RenderGroup, Delta, V2(GroundSideInMeters, GroundSideInMeters),
                                    V4(1.0f, 1.0f, 0.0f, 1.0f));
                }
            }
        }
    }
#endif    
    v3 CameraP = Subtract(World, &GameState->CameraP, &GameState->CameraP);
    
    PushRectOutline(RenderGroup, V3(0, 0, 0), GetDim(ScreenBounds), V4(1.0f, 1.0f, 0.0f, 1));
//    PushRectOutline(RenderGroup, V3(0, 0, 0), GetDim(CameraBoundsInMeters).xy, V4(1.0f, 1.0f, 1.0f, 1));

    RenderGroup->GlobalAlpha = 1.0f;
    
    TiledRenderGroupToOutput(TranState->RenderQueue, RenderGroup, DrawBuffer);

    EndTemporaryMemory(RenderMemory);
    
    CheckArena(&GameState->WorldArena);
    CheckArena(&TranState->TranArena);

    END_TIMED_BLOCK(GameUpdateAndRender)
}

