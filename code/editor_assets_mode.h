#if !defined(EDITOR_ASSETS_MODE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
#include "editor_file_builder.h"

struct added_bitmap
{
    u32 ID;
    u64 DataOffset;

    ssa_tag Tags[64];
    r32 RenderHeight;
    rectangle2 CollisionRect;
};

struct builder_bitmap_file_header
{
    u32 BitmapCount;
    added_bitmap *BitmapArray;
};

enum asset_add_mode
{
    AssetMode_Bitmap,
    AssetMode_Sound,
    AssetMode_AssetSet,
    AssetMode_SpriteSheet,
    AssetMode_TileSet,
    AssetMode_Font,
    AssetMode_Quest,
    AssetMode_BinaryFile,
};

struct edit_mode_asset
{
    asset_add_mode AssetAddMode;
    array_cursor TestCursor;

    u32 BitmapIndex;
    loaded_bitmap AddBitmap;

    u32 BMPFileCount;
    char *BMPFileNames[1024];
};

#define EDITOR_ASSETS_MODE_H
#endif
