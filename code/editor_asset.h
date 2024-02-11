#if !defined(VIEW_TILEMAP_ASSET_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct loaded_sound
{
    // TODO(casey): This could be shrunk to 12 bytes if the loaded_bitmap
    // ever got down that small
    int16 *Samples[2];
    u32 SampleCount; // NOTE(casey): This is the sample count divided by 8
    u32 ChannelCount;
};

struct loaded_font
{
    ssa_font_glyph *Glyphs;
    r32 *HorizontalAdvance;
    u32 BitmapIDOffset;
    u16 *UnicodeMap;
};

struct loaded_tileset
{
    ssa_tile *Tiles;
    u32 BitmapIDOffset;
};

struct loaded_spritesheet
{
    bitmap_id *SpriteIDs;
    u32 BitmapIDOffset;
};

struct loaded_assetset
{
    u32 *AssetIDs;
    u32 IDOffset;
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
};

enum asset_header_type
{
    AssetType_None,
    AssetType_Bitmap,
    AssetType_Sound,
    AssetType_Font,
    AssetType_Tileset,
    AssetType_Assetset,
    AssetType_SpriteSheet,
};

struct asset_memory_header
{
    asset_memory_header *Next;
    asset_memory_header *Prev;

    u32 AssetType;
    u32 AssetIndex;
    u32 TotalSize;
    u32 GenerationID;
    union
    {
        loaded_bitmap Bitmap;
        loaded_sound Sound;
        loaded_font Font;
        loaded_tileset Tileset;
        loaded_assetset Assetset;
        loaded_spritesheet SpriteSheet;
    };
};

struct asset
{
    u32 State;
    asset_memory_header *Header;
    
    ssa_asset SSA;
    u32 FileIndex;
};

struct asset_vector
{
    real32 E[Tag_Count];
};

struct asset_type
{
    uint32 FirstAssetIndex;
    uint32 OnePastLastAssetIndex;
};

struct asset_file
{
    platform_file_handle Handle;
    ssa_header Header;
    ssa_asset_type *AssetTypeArray;

    u32 TagBase; 
    
    u32 AssetTypeOffsets[Asset_Count];
};

enum asset_memory_block_flags
{
    AssetMemory_Used = 0x1,
};

struct asset_memory_block
{
    asset_memory_block *Prev;
    asset_memory_block *Next;

    u64 Flags;
    memory_index Size;
};

struct game_assets
{
    u32 NextGenerationID;

    struct transient_state *TranState;

    asset_memory_block MemorySentinel;
    
    asset_memory_header LoadedAssetSentinel;
    
    real32 TagRange[Tag_Count];

    u32 FileCount;
    asset_file *Files;

    uint32 TagCount;
    ssa_tag *Tags;
    
    uint32 AssetCount;
    asset *Assets;
    
    asset_type AssetTypes[Asset_Count];

    u32 OperationLock;

    u32 InFlightGenerationCount;
    u32 InFlightGenerations[16];
};

inline void
BeginAssetLock(game_assets *Assets)
{
    for(;;)
    {
        if(AtomicCompareExchangeUInt32(&Assets->OperationLock, 1, 0) == 0)
        {
            break;
        }
    }
}

inline void
EndAssetLock(game_assets *Assets)
{
    CompletePreviousWritesBeforeFutureWrites;
    Assets->OperationLock = 0;
}

inline void
InsertAssetHeaderAtFront(game_assets *Assets, asset_memory_header *Header)
{
    asset_memory_header *Sentinel = &Assets->LoadedAssetSentinel;

    Header->Prev = Sentinel;
    Header->Next = Sentinel->Next;

    Header->Next->Prev = Header;
    Header->Prev->Next = Header;
}

inline void
RemoveAssetHeaderFromList(asset_memory_header *Header)
{
    Header->Prev->Next = Header->Next;
    Header->Next->Prev = Header->Prev;

    Header->Next = Header->Prev = 0;
}

internal void MoveHeaderToFront(game_assets *Assets, asset *Asset);
inline asset_memory_header *
GetAsset(game_assets *Assets, u32 ID, u32 GenerationID)
{
    Assert(ID <= Assets->AssetCount);
    asset *Asset = Assets->Assets + ID;

    asset_memory_header *Result = 0;

    BeginAssetLock(Assets);

    if(Asset->State == AssetState_Loaded)
    {
        Result = Asset->Header;
    
        RemoveAssetHeaderFromList(Result);
        InsertAssetHeaderAtFront(Assets, Result);

        if(Asset->Header->GenerationID < GenerationID)
        {
            Asset->Header->GenerationID = GenerationID;
        }
                
        CompletePreviousWritesBeforeFutureWrites;
    }

    EndAssetLock(Assets);
    
    return(Result);
}

inline loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_bitmap *Result = Header ? &Header->Bitmap : 0;
    
    return(Result);
}

inline ssa_bitmap *
GetBitmapInfo(game_assets *Assets, bitmap_id ID)
{
    Assert((u32)ID.Value <= Assets->AssetCount);
    ssa_bitmap *Result = &Assets->Assets[ID.Value].SSA.Bitmap;

    return(Result);
}

inline loaded_sound *
GetSound(game_assets *Assets, sound_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_sound *Result = Header ? &Header->Sound : 0;

    return(Result);
}

inline ssa_sound *
GetSoundInfo(game_assets *Assets, sound_id ID)
{
    Assert((u32)ID.Value <= Assets->AssetCount);
    ssa_sound *Result = &Assets->Assets[ID.Value].SSA.Sound;

    return(Result);
}

inline loaded_font *
GetFont(game_assets *Assets, font_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_font *Result = Header ? &Header->Font : 0;

    return(Result);
}

inline ssa_font *
GetFontInfo(game_assets *Assets, font_id ID)
{
    Assert((u32)ID.Value <= Assets->AssetCount);
    ssa_font *Result = &Assets->Assets[ID.Value].SSA.Font;

    return(Result);
}

inline loaded_tileset *
GetTileset(game_assets *Assets, tileset_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_tileset *Result = Header ? &Header->Tileset : 0;

    return(Result);
}

inline ssa_tileset *
GetTilesetInfo(game_assets *Assets, tileset_id ID)
{
    Assert((u32)ID.Value <= Assets->AssetCount);
    ssa_tileset *Result = &Assets->Assets[ID.Value].SSA.Tileset;

    return(Result);
}

inline loaded_assetset *
GetAssetset(game_assets *Assets, assetset_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_assetset *Result = Header ? &Header->Assetset : 0;

    return(Result);
}

inline ssa_assetset *
GetAssetsetInfo(game_assets *Assets, assetset_id ID)
{
    Assert((u32)ID.Value <= Assets->AssetCount);
    ssa_assetset *Result = &Assets->Assets[ID.Value].SSA.Assetset;

    return(Result);
}

inline loaded_spritesheet *
GetSpriteSheet(game_assets *Assets, spritesheet_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_spritesheet *Result = Header ? &Header->SpriteSheet : 0;

    return(Result);
}

inline ssa_spritesheet *
GetSpriteSheetInfo(game_assets *Assets, spritesheet_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    ssa_spritesheet *Result = &Assets->Assets[ID.Value].SSA.SpriteSheet;

    return(Result);
}

inline bool32
IsValid(bitmap_id ID)
{
    bool32 Result = (ID.Value != 0);

    return(Result);
}

inline bool32
IsValid(sound_id ID)
{
    bool32 Result = (ID.Value != 0);

    return(Result);
}

internal void LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate);
inline void PrefetchBitmap(game_assets *Assets, bitmap_id ID) {LoadBitmap(Assets, ID, false);}

internal void LoadSound(game_assets *Assets, sound_id ID);
inline void PrefetchSound(game_assets *Assets, sound_id ID) {LoadSound(Assets, ID);}

internal void LoadFont(game_assets *Assets, font_id ID, b32 Immediate);
inline void PrefetchFont(game_assets *Assets, font_id ID) {LoadFont(Assets, ID, false);}

internal void LoadTileset(game_assets *Assets, tileset_id ID, b32 Immediate);
inline void PrefetchTileset(game_assets *Assets, tileset_id ID) {LoadTileset(Assets, ID, false);}

internal void LoadAssetset(game_assets *Assets, assetset_id ID, b32 Immediate);
inline void PrefetchAssetset(game_assets *Assets, assetset_id ID) {LoadAssetset(Assets, ID, false);}

internal void LoadSpriteSheet(game_assets *Assets, spritesheet_id ID, b32 Immediate);
inline void PrefetchSpriteSheet(game_assets *Assets, spritesheet_id ID) {LoadSpriteSheet(Assets, ID, false);}

inline sound_id
GetNextSoundInChain(game_assets *Assets, sound_id ID)
{
    sound_id Result = {};

    ssa_sound *Info = GetSoundInfo(Assets, ID); 

    switch(Info->Chain)
    {
        case SSASoundChain_None:
        {
            // NOTE(casey): Nothing to do
        } break;

        case SSASoundChain_Loop:
        {
            Result = ID;
        } break;

        case SSASoundChain_Advance:
        {
            Result.Value = ID.Value + 1;
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }
    
    return(Result);
}

inline u32
BeginGeneration(game_assets *Assets)
{
    BeginAssetLock(Assets);

    Assert(Assets->InFlightGenerationCount < ArrayCount(Assets->InFlightGenerations));
    u32 Result = Assets->NextGenerationID++;
    Assets->InFlightGenerations[Assets->InFlightGenerationCount++] = Result;
    
    EndAssetLock(Assets);
    
    return(Result);
}

inline void
EndGeneration(game_assets *Assets, u32 GenerationID)
{
    BeginAssetLock(Assets);

    for(u32 InFlightGenerationIndex = 0;
        InFlightGenerationIndex < Assets->InFlightGenerationCount;
        ++InFlightGenerationIndex)
    {
        if(Assets->InFlightGenerations[InFlightGenerationIndex] == GenerationID)
        {
            Assets->InFlightGenerations[InFlightGenerationIndex] =
                Assets->InFlightGenerations[--Assets->InFlightGenerationCount];
            break;
        }
    }

    EndAssetLock(Assets);
}

#define VIEW_TILEMAP_ASSET_H
#endif
