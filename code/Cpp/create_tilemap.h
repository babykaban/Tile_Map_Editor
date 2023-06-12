#if !defined(CREATE_TILEMAP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#define TILESIDE 30

typedef struct debug_read_file_result
{
    u32 ContentsSize;
    void *Contents;
} debug_read_file_result;

struct loaded_bitmap
{
    i32 Width;
    i32 Height;
    u32 *Pixels;
};

struct bit_scan_result
{
    b32 Found;
    u32 Index;
};

struct arguments
{
    u32 *Colors;
    i32 ColorCount;
};

struct tile
{
    u32 TileColor;
    u16 TileIndex;
    u16 TileZ;
};



u32 MainColors[14] =
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

#define CREATE_TILEMAP_H
#endif
