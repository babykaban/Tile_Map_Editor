#if !defined(EDITOR_FILE_BUILDER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#define ONE_PAST_MAX_FONT_CODEPOINT (0x10FFFF + 1)
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 SizeOfBitmap;
    int32 HorzResolution;
    int32 VertResolution;
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};

struct WAVE_header
{
    uint32 RIFFID;
    uint32 Size;
    uint32 WAVEID;
};

#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
enum
{
    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};

struct WAVE_chunk
{
    uint32 ID;
    uint32 Size;
};

struct WAVE_fmt
{
    uint16 wFormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;
    uint16 cbSize;
    uint16 wValidBitsPerSample;
    uint32 dwChannelMask;
    uint8 SubFormat[16];
};

struct builder_loaded_sound
{
    uint32 SampleCount; // NOTE(casey): This is the sample count divided by 8
    uint32 ChannelCount;
    int16 *Samples[2];
};

struct builder_loaded_bitmap
{
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
};

struct builder_loaded_font
{
//    HFONT Win32Handle;
//    TEXTMETRIC TextMetric;
    r32 LineAdvance;

    ssa_font_glyph *Glyphs;
    r32 *HorizontalAdvance;

    u32 MinCodePoint;
    u32 MaxCodePoint;

    u32 MaxGlyphCount;
    u32 GlyphCount;

    u32 *GlyphIndexFromCodePoint;
    u32 OnePastHighestCodePoint;
};

struct builder_loaded_tileset
{
    u32 TileCount;
    ssa_tile *Tiles;
    u32 *TileOffsetX;
    u32 *TileOffsetY;
    
    char *TilesetFileName;
    u32 Biome;
    u32 TileType;
    u32 Height;
    u32 CliffHillType;
    u32 MainTileSurface;
    u32 MergeTileSurface;
    loaded_bitmap *MergeTile;
};

struct builder_loaded_spritesheet
{
    u32 SpriteCount;
    bitmap_id *SpriteIDs;

    loaded_bitmap Sprites[64];
    r32 SpriteAlignX;
    r32 SpriteAlignY;
};

struct builder_loaded_assetset
{
    u32 AssetCount;
    u32 AssetType;
    u32 AssetIDs[256];
};

struct builder_loaded_text
{
    u32 Length;
    char *String;
};

struct builder_loaded_quest
{
    u32 GiverParCount;
    u32 ObjectiveParCount;
    u32 ComplitionParCount;

    u32 GiverTextIDs[64];
    u32 ObjectiveTextIDs[64];
    u32 ComplitionTextIDs[64];
};

struct builder_loaded_binary_file
{
    u32 Size;
    void *Data;
};

enum builder_asset_type
{
    BuilderAssetType_Sound,
    BuilderAssetType_Bitmap,
    BuilderAssetType_Font,
    BuilderAssetType_FontGlyph,
    BuilderAssetType_Tile,
    BuilderAssetType_Tileset,
    BuilderAssetType_SpriteSheet,
    BuilderAssetType_Sprite,
    BuilderAssetType_AssetSet,
    BuilderAssetType_Text,
    BuilderAssetType_Quest,
    BuilderAssetType_BinaryFile,
};

struct builder_loaded_font;
struct asset_source_font
{
    builder_loaded_font *Font;
};

struct asset_source_font_glyph
{
    builder_loaded_font *Font;
    u32 CodePoint;
};

struct asset_source_sound
{
    char *FileName;
    u32 FirstSampleIndex;
};

struct asset_source_bitmap
{
    char *FileName;
};

struct asset_source_tile
{
    u32 MainTileSurface;
    u32 MergeTileSurface;
    u32 TileIndex;
    builder_loaded_tileset *Tileset;
};

struct asset_source_tileset
{
    builder_loaded_tileset *Tileset;
};

struct asset_source_sprite
{
    builder_loaded_bitmap Bitmap;
};

struct asset_source_spritesheet
{
    builder_loaded_spritesheet *Sheet;
};

struct asset_source_assetset
{
    builder_loaded_assetset *AssetSet;
};

struct asset_source_text
{
    builder_loaded_text *Text;
};

struct asset_source_quest
{
    builder_loaded_quest *Quest;
};

struct asset_source_binary_file
{
    char *FileName;
};

struct asset_source
{
    asset_type Type;
    union
    {
        asset_source_bitmap Bitmap;
        asset_source_sound Sound;
        asset_source_font Font;
        asset_source_font_glyph Glyph;
        asset_source_tile Tile;
        asset_source_tile Tileset;
        asset_source_sprite Sprite;
        asset_source_spritesheet SpriteSheet;
        asset_source_assetset AssetSet;
        asset_source_text Text;
        asset_source_quest Quest;
        asset_source_binary_file File;
    };
};

#define VERY_LARGE_NUMBER 4096*32
struct builder_assets
{
    b32 Initialized;
    
    uint32 TagCount;
    ssa_tag *Tags;

    u32 AssetTypeCount;
    ssa_asset_type AssetTypes[Asset_Count];

    uint32 AssetCount;
    asset_source *AssetSources;
    ssa_asset *Assets;

    ssa_asset_type *AddAssetType;
    u32 AssetIndex;
};

#pragma pack(pop)

#define EDITOR_FILE_BUILDER_H
#endif
