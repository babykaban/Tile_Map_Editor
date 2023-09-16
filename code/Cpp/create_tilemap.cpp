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

struct chunk
{
    int32 ChunkX;
    int32 ChunkY;

    uint32 TileID[16];
};

struct info
{
    uint32 ChunkCountX;
    uint32 ChunkCountY;

    uint32 TileIndecies[1024];
};

#if 0
static info
TakeInfo(loaded_bitmap *Bitmap)
{

    for(int32 ChunkIndexY = 0;
        ChunkIndexY < Result.ChunkCountY;
        ++ChunkIndexY)
    {
        for(int32 ChunkIndexX = 0;
            ChunkIndexX < Result.ChunkCountX;
            ++ChunkIndexX)
        {
            int32 ChunkIndex = ChunkIndexY*TILES_PER_CHUNK + ChunkIndexX;
            chunk *Chunk = &Result.Chunks + ChunkIndex;
            
        }
    }
    
    return(Result);
}
#endif

struct tile_identities
{
    uint32 Count;
    uint32 Indecies[256];
    uint32 Colors[256];
};

static info
TakeInfo(tile_identities *Identities, loaded_bitmap *Bitmap)
{
    info Result = {};

    Result.ChunkCountX = Bitmap->Width % TILES_PER_CHUNK ? (Bitmap->Width / TILES_PER_CHUNK) + 1 :
        Bitmap->Width / TILES_PER_CHUNK;

    Result.ChunkCountY = Bitmap->Height % TILES_PER_CHUNK ? (Bitmap->Height / TILES_PER_CHUNK) + 1 :
        Bitmap->Height / TILES_PER_CHUNK;

    uint32 *TileIdentities = (uint32 *)Bitmap->Memory;

    for(int32 TileIndex = 0;
        TileIndex < Bitmap->Height * Bitmap->Width;
        ++TileIndex)
    {
        uint32 *TileIndentity = TileIdentities + TileIndex;

        uint32 TileID = (uint32)BinarySearch(Identities->Colors, Identities->Count, *TileIndentity);

        if(TileID)
        {
            Result.TileIndecies[TileIndex] = TileID;
        }
    }
    
    return(Result);
}

static tile_identities Identities;

static void
LoadTileDataAndColors(uint32 *Indecies, uint32 *Colors)
{
    uint32 FileContent[512] = {};
    int32 DWordsReaded = ReadBinaryFile32("output_0.bin", FileContent);
    
    int32 Count = 0;
    for(int32 ItemIndex = 0;
        ItemIndex <= DWordsReaded;
        ItemIndex += 2)
    {
        *Indecies = FileContent[ItemIndex];
        *Colors = FileContent[ItemIndex + 1];

        *Colors++;
        *Indecies++;
        ++Count;
    }

    Identities.Count = Count;
}

int main()
{
    LoadTileDataAndColors(Identities.Indecies, Identities.Colors);
    
    loaded_bitmap Source = {};
    Source = LoadBMP("tiles\\structured_art.bmp");
    
    return(0);
}



