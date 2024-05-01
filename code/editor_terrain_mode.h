#if !defined(EDITOR_TERRAIN_MODE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct tileset_stats
{
    u8 Biome;
    u8 Type;
    u8 Height;
    u8 CliffHillType;
    u8 MainSurface;
    u8 MergeSurface;
};

struct world_tile
{
    u32 TileID;
    bitmap_id TileBitmapID;
};

struct edit_mode_terrain
{
    tileset_stats TileSetStats;
    array_cursor TileMenuBarCursor;

    b32 WorldTilesInitialized;
    u32 WorldTileCount;
    u32 *TileIDs;
    world_tile *WorldTiles;
    
    tileset_id GlobalTilesetID;
};

#define EDITOR_TERRAIN_MODE_H
#endif
