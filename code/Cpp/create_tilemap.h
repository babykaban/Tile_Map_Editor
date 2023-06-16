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

struct colors
{
    u32 *Colors;
    i32 ColorCount;
};

struct tile
{
    u32 Color;
    u16 Index;
    u16 Z;
};


#define CREATE_TILEMAP_H
#endif
