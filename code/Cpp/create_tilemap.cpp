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
    uint32 Array[256];
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

        uint32 TileID = (uint32)BinarySearch(Identities->Array, Identities->Count, *TileIndentity);

        if(TileID)
        {
            Result.TileIndecies[TileIndex] = TileID;
        }
    }
    
    return(Result);
}


int main()
{
//    chunk Chunks[4096];

    loaded_bitmap Source = {};
    Source = LoadBMP("tiles\\structured_art.bmp");
    
    return(0);
}



