#if !defined(VIEW_TILEMAP_FILE_FORMATS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

enum asset_font_type
{
    FontType_Default = 0,
    FontType_Nice = 1,
    FontType_Debug = 10,
    FontType_DebugSmall = 11,
};

enum tile_biome_type
{
    BiomeType_AncientForest,
    BiomeType_DeepCave,
    BiomeType_DesertV1,
    BiomeType_DesertV2,
    BiomeType_GrassLand,
    BiomeType_MarshlandV1,
    BiomeType_MarshlandV2,
    BiomeType_Mountains,
    BiomeType_Strangeland,
    BiomeType_Wetland,

    BiomeType_Count,
};

enum tile_state
{
    TileState_Solid,

    TileState_TopLeftCornerOutside,
    TileState_BottomLeftCornerOutside,
    TileState_TopRightCornerOutside,
    TileState_BottomRightCornerOutside,

    TileState_TopLeftCornerInside,
    TileState_BottomLeftCornerInside,
    TileState_TopRightCornerInside,
    TileState_BottomRightCornerInside,

    TileState_TopVertical0,
    TileState_TopVertical1,
    TileState_TopVertical2,

    TileState_BottomVertical0,
    TileState_BottomVertical1,
    TileState_BottomVertical2,

    TileState_LeftHorizontal0,
    TileState_LeftHorizontal1,
    TileState_LeftHorizontal2,

    TileState_RightHorizontal0,
    TileState_RightHorizontal1,
    TileState_RightHorizontal2,

    TileState_Count,
};

enum tile_surface
{
    TileSurface_0,
    TileSurface_1,
    TileSurface_2,
    TileSurface_3,
    TileSurface_4,
    TileSurface_5,
    TileSurface_6,
    TileSurface_7,
    TileSurface_8,
    TileSurface_9,
    TileSurface_10,
    TileSurface_11,
    TileSurface_12,
    TileSurface_13,
    TileSurface_14,
    TileSurface_15,

    TileSurface_Count,
};

enum asset_tag_id
{
    Tag_FacingDirection, // NOTE(casey): Angle in radians off of due right
    Tag_UnicodeCodepoint,
    Tag_FontType,

    Tag_TileBiomeType,
    Tag_TileState,
    Tag_TileSurface,
    
    Tag_Count,
};

enum asset_type_id
{
    Asset_None,

    //
    // NOTE(casey): Bitmaps!
    //
    
    Asset_Shadow,
    Asset_Tree,
    Asset_Sword,
    Asset_Rock,

    Asset_Grass,
    Asset_Tuft,
    Asset_Stone,

    Asset_Head,
    Asset_Cape,
    Asset_Torso,

    Asset_Font,
    Asset_FontGlyph,

    Asset_Tile,
    //
    // NOTE(casey): Sounds!
    //

    Asset_Bloop,
    Asset_Crack,
    Asset_Drop,
    Asset_Glide,
    Asset_Music,
    Asset_Puhp,

    //
    //
    //
    
    Asset_Count
};

#define SSA_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

#pragma pack(push, 1)

struct bitmap_id
{
    u32 Value;
};

struct sound_id
{
    u32 Value;
};

struct font_id
{
    u32 Value;
};

struct ssa_header
{
#define SSA_MAGIC_VALUE SSA_CODE('s', 's', 'a', 'f')
    u32 MagicValue;
#define SSA_VERSION 0
    u32 Version;

    u32 TagCount;
    u32 AssetTypeCount;
    u32 AssetCount;

    u64 Tags; // ssa_tag[TagCount]
    u64 AssetTypes; // ssa_asset_type[AssetTypeCount]
    u64 Assets; // ssa_asset[AssetCount]
};

struct ssa_tag
{
    u32 ID;
    r32 Value;
};

struct ssa_asset_type
{
    u32 TypeID;
    u32 FirstAssetIndex;
    u32 OnePastLastAssetIndex;
};

enum ssa_sound_chain
{
    SSASoundChain_None,
    SSASoundChain_Loop,
    SSASoundChain_Advance,
};

struct ssa_bitmap
{
    u32 Dim[2];
    r32 AlignPercentage[2];

    /* NOTE(casey): Data is:

       u32 Pixels[Dim[1]][Dim[0]]
    */
};

struct ssa_sound
{
    u32 SampleCount;
    u32 ChannelCount;
    u32 Chain; // NOTE(casey): ssa_sound_chain

    /* NOTE(casey): Data is:

       s16 Channels[ChannelCount][SampleCount]
    */
};

struct ssa_font_glyph
{
    u32 UnicodeCodePoint;
    bitmap_id BitmapID;
};

struct ssa_font
{
    u32 OnePastHighestCodePoint;
    u32 GlyphCount;
    r32 AscenderHeight;
    r32 DescenderHeight;
    r32 ExternalLeading;

    /* NOTE(casey): Data is:

       ssa_font_glyph CodePoints[GlyphCount];
       r32 HorizontalAdvance[GlyphCount][GlyphCount];
    */
};

struct ssa_asset
{
    u64 DataOffset;
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;
    union
    {
        ssa_bitmap Bitmap;
        ssa_sound Sound;
        ssa_font Font;
    };
};

#pragma pack(pop)

#define VIEW_TILEMAP_FILE_FORMATS_H
#endif
