#if !defined(CREATE_TILEMAP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

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
    i32 TileSide;
    u32 *Colors;
    u32 ColorCount;
};

#define CREATE_TILEMAP_H
#endif
