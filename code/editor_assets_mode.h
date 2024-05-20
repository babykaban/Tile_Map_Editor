#if !defined(EDITOR_ASSETS_MODE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
#include "editor_file_builder.h"

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

struct add_asset_source_bitmap
{
    char *FileName;
    v2 AlignPercentage;
    r32 RenderHeight;
};

struct add_asset_source
{
    builder_asset_type AssetType;
    union
    {
        add_asset_source_bitmap AddBitmap;
    };
};

struct asset_to_add
{
    asset_type_id TypeID;
    u32 TagCount;
    ssa_tag Tags[256];
    add_asset_source Source;
};

struct edit_mode_asset
{
    b32 AddAsset;

    asset_add_mode AssetAddMode;
    array_cursor BMPCursor;
    array_cursor TagCursor;
    array_cursor AssetTypeCursor;

    builder_assets BuilderAssets;
    
    u32 BMPFileCount;
    char *BMPFileNames[1024];

    b32 PlaceAlignment;
    b32 AddTag;
    asset_to_add AssetToAdd;
    u32 BitmapIndex;
    loaded_bitmap AddBitmap;
};

#define EDITOR_ASSETS_MODE_H
#endif
