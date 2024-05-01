#if !defined(VIEW_TILEMAP_WORLD_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   ======================================================================== */

struct tile_position
{
    int32 TileX;
    int32 TileY;
};

struct world_position
{
    // TODO(casey): It seems like we have to store ChunkX/Y/Z with each
    // entity because even though the sim region gather doesn't need it
    // at first, and we could get by without it, entity references pull
    // in entities WITHOUT going throuugh their world_chunk, and thus
    // still need to know the ChunkX/Y/Z
    
    int32 ChunkX;
    int32 ChunkY;

    // NOTE(casey): These are the offset from the chunk center
    v2 Offset_;
};

struct world_chunk
{
    int32 ChunkX;
    int32 ChunkY;
    
    world_chunk *NextInHash;
};

struct world
{
    memory_arena Arena;

    v2 ChunkDimInMeters;

    world_position CameraP;
    world_position CameraBoundsMin;
    world_position CameraBoundsMax;

    b32 RenderGround;
    b32 RenderDecorations;
    b32 ShowBorders;

    b32 AllowEdit;

    // NOTE(casey): At the moment, this must be a power of two!
    world_chunk ChunkHash[4096];
};

#define VIEW_TILEMAP_WORLD_H
#endif
