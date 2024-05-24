#if !defined(EDITOR_DECORATION_MODE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct assetset_stats
{
    u8 Biome;
    u8 Type;
};

struct decoration
{
    world_position P;

    u32 AssetTypeID;
    b32 IsSpriteSheet;
    u32 DecorationIndex;

    r32 Height;
    u32 TagCount;
    ssa_tag Tags[64];

    union
    {
        bitmap_id BitmapID;
        spritesheet_id SpriteSheetID;
    };
};

struct animated_decoration
{
    u32 SpriteIndex;
    ssa_spritesheet *Info;
    loaded_spritesheet *SpriteSheet;
};

struct edit_mode_decoration
{
    assetset_stats AssetSetStats;
//    array_cursor AssetMenuBarCursor;

    decoration *Decorations;
    // NOTE(paul): Only for drawing
    animated_decoration *AnimatedDecorations;

    r32 Time;
};

#define EDITOR_DECORATION_MODE_H
#endif
