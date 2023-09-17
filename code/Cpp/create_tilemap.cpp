/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

#include "main_platform.h"
#include "main_platform.cpp"

#include "create_tilemap.h"


#define TILES_PER_CHUNK 8
#define BITMAP_BYTES_PER_PIXEL 4

struct loaded_tile
{
    uint32 Identity;
    loaded_bitmap Bitmap;
};

static void
ClearBitmap(loaded_bitmap *Bitmap)
{
    if(Bitmap->Memory)
    {
        int32 TotalBitmapSize = Bitmap->Width*Bitmap->Height*BITMAP_BYTES_PER_PIXEL;
        ZeroSize(TotalBitmapSize, Bitmap->Memory);
    }
}

static loaded_bitmap
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

static uint32
LoadTileDataAndIdentities(memory_arena *MainArena, loaded_tile *Tiles, loaded_bitmap *TileSheet, int32 TileDim)
{
    uint32 TileCountX = (TileSheet->Width / TileDim);
    uint32 TileCountY = (TileSheet->Height / TileDim);

    uint32 LoadedTileCount = 0; 
    for(uint32 TileY = 0;
        TileY < TileCountY;
        ++TileY)
    {
    
        for(uint32 TileX = 0;
            TileX < TileCountX;
            ++TileX)
        {
            loaded_tile *Tile = Tiles + LoadedTileCount;
            Tile->Bitmap = MakeEmptyBitmap(MainArena, TileDim, TileDim);
            loaded_bitmap *Bitmap = &Tile->Bitmap;
            ++LoadedTileCount;

            int32 MinX = TileX * TileDim;
            int32 MinY = TileY * TileDim;
            int32 MaxX = MinX + TileDim;
            int32 MaxY = MinY + TileDim;

            uint8 *SourceRow = ((uint8 *)TileSheet->Memory + MinX*BITMAP_BYTES_PER_PIXEL + MinY*TileSheet->Pitch);
    
            uint8 *DestRow = (uint8 *)Bitmap->Memory;
            uint64 ColorSum = 0;
            for(int Y = MinY;
                Y < MaxY;
                ++Y)
            {
                uint32 *Dest = (uint32 *)DestRow;
                uint32 *Source = (uint32 *)SourceRow;
                for(int X = MinX;
                    X < MaxX;
                    ++X)
                {
                    *Dest = *Source;

                    ColorSum += (uint64)*Dest;
                    ++Dest;
                    ++Source;
                }

                DestRow += Bitmap->Pitch;
                SourceRow += TileSheet->Pitch;
            }

            Tile->Identity = (uint32)(ColorSum / (TileDim*TileDim));
        }
    }

    return(LoadedTileCount);
}

int main()
{
    LPVOID BaseAddress = 0;
    uint64 MainMemoryStorageSize = Megabytes(256);
    void *MainMemoryStorage = VirtualAlloc(BaseAddress, (memory_index)MainMemoryStorageSize,
                                         MEM_RESERVE|MEM_COMMIT,
                                         PAGE_READWRITE);

    memory_arena MainArena;
    InitializeArena(&MainArena, (memory_index)MainMemoryStorageSize, MainMemoryStorage);
    
    uint32 LoadedTileCount = 0;
    loaded_tile *Tiles = PushArray(&MainArena, 256, loaded_tile);

    int32 TileDim = 60;
    loaded_bitmap TileSheet = LoadBMP("tile_sheet.bmp");
    TileSheet.Pitch = TileSheet.Width*BITMAP_BYTES_PER_PIXEL;

    if(TileSheet.Memory)
    {
        LoadedTileCount = LoadTileDataAndIdentities(&MainArena, Tiles, &TileSheet, TileDim);
    
        loaded_bitmap MapSource = {};
        MapSource = LoadBMP("tiles\\structured_art.bmp");

//        AsignTilesForChunks(Tiles, LoadedTileCount, );
    }
    
    return(0);
}



