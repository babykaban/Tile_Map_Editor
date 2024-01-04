/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

global_variable assetset_id AssetsetID;

internal void
WriteDecorations(char *FileName, u32 Count, decoration *Decorations)
{
    uint32 ContentSize = sizeof(decoration)*Count;
    Platform.DEBUGWriteEntireFile(0, FileName, ContentSize, Decorations);
}

internal void
ShowAssetMenuBar(render_group *RenderGroup, array_cursor *Cursor, r32 TileDimInMeters)
{
    loaded_assetset *Assetset = PushAssetset(RenderGroup, AssetsetID);
    u32 AssetCountInBar = Cursor->ArrayCount;

    r32 MenuBarWidth = 3.0f;
    r32 HalfWidth = 0.5f*MenuBarWidth;

    r32 CursorWidth = MenuBarWidth;
    r32 CursorHeight = 0.3f;
    r32 CursorHalfHeight = 0.5f*CursorHeight;

    r32 MenuBarHeight = Cursor->ArrayCount*CursorHeight;
    r32 HalfHeight = 0.5f*MenuBarHeight;
    
    r32 OffsetY = 5.5f - HalfHeight;
    r32 OffsetX = -15.5f + HalfWidth;
    
    PushRectOutline(RenderGroup, V3(OffsetX, OffsetY, 0), V2(MenuBarWidth, MenuBarHeight), V4(0, 1, 0, 1), 0.05f);

    ssa_assetset *Info = GetAssetsetInfo(RenderGroup->Assets, AssetsetID);
    if(Assetset)
    {
        r32 TileOffsetX = OffsetY - HalfWidth;
        r32 TileOffsetY = -HalfWidth;
        for(u32 Index = 0;
            Index < AssetCountInBar;
            ++Index)
        {
            u32 AssetIndex = Cursor->Array[Index];
        }
    }

    r32 CursorOffsetY = (HalfHeight + OffsetY - CursorHalfHeight) - Cursor->ArrayPosition*CursorHeight;
    PushRectOutline(RenderGroup, V3(OffsetX, CursorOffsetY, 0), V2(CursorWidth, CursorHeight), V4(1, 1, 1, 1), 0.03f);
}

internal void
ShowAssetsetStats(render_group *RenderGroup, game_state *GameState)
{
    DEBUGTextLine(RenderGroup, "Assetset Stats:");
    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer), "Biome: %s", Biomes[GameState->AssetSetStats.Biome]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "AssetType: %s", AssetTypes[GameState->AssetSetStats.Type]);
    DEBUGTextLine(RenderGroup, TextBuffer);
}

internal void
ReloadAssetset(game_assets *Assets, game_state *GameState)
{
    asset_vector WeightVector = {};
    WeightVector.E[Tag_BiomeType] = 1.0f;
    WeightVector.E[Tag_AssetType] = 1.0f;

    asset_vector MatchVector = {};
    MatchVector.E[Tag_BiomeType] = (r32)GameState->AssetSetStats.Biome;
    MatchVector.E[Tag_AssetType] = (r32)GameState->AssetSetStats.Type;
    
    assetset_id ID = GetBestMatchAssetsetFrom(Assets, Asset_AssetSet, &MatchVector, &WeightVector);
    if(ID.Value != AssetsetID.Value)
    {
        AssetsetID = ID;
        ssa_assetset *Info = GetAssetsetInfo(Assets, AssetsetID);
        if((Info->AssetCount > 10))
        {
            ReInitializeCursor(&GameState->AssetMenuBarCursor, 10);
        }
        else
        {
            ReInitializeCursor(&GameState->AssetMenuBarCursor, Info->AssetCount);
        }
    }
}

internal void
AddDecoration(render_group *RenderGroup, game_state *GameState, world_position *MouseP, r32 PixelsToMeters)
{
    u32 DecorationIndex = GetTileIndexFromChunkPosition(GameState, MouseP);
    if((MouseP->ChunkX >= 0) && (MouseP->ChunkY >= 0))
    {
        loaded_assetset *Assetset = PushAssetset(RenderGroup, AssetsetID);
        if(Assetset)
        {
            u32 AssetIndex = GameState->AssetMenuBarCursor.Array[GameState->AssetMenuBarCursor.ArrayPosition];

            bitmap_id ID = {}; 
            ID.Value = Assetset->AssetIDs[AssetIndex] + Assetset->IDOffset;

            ssa_bitmap *BitmapInfo = GetBitmapInfo(RenderGroup->Assets, ID);
            v2 BitmapDimInMeters = PixelsToMeters*V2i(BitmapInfo->Dim[0], BitmapInfo->Dim[1]);

            asset_tag_result Tags = GetAssetTags(RenderGroup->Assets, ID.Value);
            GameState->Decorations[DecorationIndex].MatchVector = Tags.MatchVector;
            GameState->Decorations[DecorationIndex].WeightVector = Tags.WeightVector;
            
            GameState->Decorations[DecorationIndex].BitmapID = ID;
            GameState->Decorations[DecorationIndex].Height = BitmapDimInMeters.y;

            tile_position TileP = TilePositionFromChunkPosition(MouseP);
            GameState->Decorations[DecorationIndex].P =
                ChunkPositionFromTilePosition(GameState->World, TileP.TileX, TileP.TileY);
            
        }
    }
}

internal void
RemoveDecoration(render_group *RenderGroup, game_state *GameState, world_position *MouseP, r32 PixelsToMeters)
{
    u32 DecorationIndex = GetTileIndexFromChunkPosition(GameState, MouseP);
    if((MouseP->ChunkX >= 0) && (MouseP->ChunkY >= 0))
    {
        GameState->Decorations[DecorationIndex].BitmapID.Value = 0;
        GameState->Decorations[DecorationIndex].Height = 0;
        GameState->Decorations[DecorationIndex].P = {};
    }
}

internal void
DecorationEditMode(render_group *RenderGroup, render_group *TextRenderGroup, game_state *GameState,
                   transient_state *TranState, game_input *Input, world_position *MouseChunkP,
                   r32 TileSideInMeters, r32 PixelsToMeters)
{

    if(Input->MouseButtons[0].EndedDown)
    {
        AddDecoration(RenderGroup, GameState, MouseChunkP, PixelsToMeters);
    }

    if(Input->MouseButtons[2].EndedDown)
    {
        RemoveDecoration(RenderGroup, GameState, MouseChunkP, PixelsToMeters);
    }

    ssa_assetset *Info = GetAssetsetInfo(TranState->Assets, AssetsetID);
    ChangeCursorPositionFor(&GameState->AssetMenuBarCursor, Info->AssetCount, Input->MouseZ);
    ShowTest(TextRenderGroup, &GameState->AssetMenuBarCursor);
    ShowAssetMenuBar(RenderGroup, &GameState->AssetMenuBarCursor, TileSideInMeters);
    ShowAssetsetStats(TextRenderGroup, GameState);


    WriteDecorations("decorations.bin", GameState->WorldTileCount, GameState->Decorations);
    ReloadAssetset(TranState->Assets, GameState);


    v2 Delta = Subtract(GameState->World, &GameState->CameraP, MouseChunkP);
    PushRect(RenderGroup, V3(-Delta, 0),
             0.2f*V2(TileSideInMeters, TileSideInMeters), V4(0, 0, 1, 1));

    tile_position Tp = TilePositionFromChunkPosition(MouseChunkP);
    tile_position TCp = TilePositionFromChunkPosition(&GameState->CameraP);
    

    v2 dTile =
        {
            (real32)TCp.TileX - (real32)Tp.TileX,
            (real32)TCp.TileY - (real32)Tp.TileY
        };

    v2 D = dTile*TileSideInMeters - V2(0.5f, 0.5f);

    PushRectOutline(RenderGroup, V3(-D, 0), V2(TileSideInMeters, TileSideInMeters),
                    V4(0.0f, 0.0f, 1.0f, 1), 0.02f);

    loaded_assetset *Assetset = PushAssetset(RenderGroup, AssetsetID, true);
    ssa_assetset *AssetsetInfo = GetAssetsetInfo(TranState->Assets, AssetsetID);
    if(Assetset && AssetsetInfo)
    {
        u32 ArrayIndex = GameState->AssetMenuBarCursor.Array[GameState->AssetMenuBarCursor.ArrayPosition];
        bitmap_id BitmapID = GetBitmapFromAssetset(TranState->Assets, AssetsetInfo, Assetset,
                                                   ArrayIndex);

        ssa_bitmap *BitmapInfo = GetBitmapInfo(TranState->Assets, BitmapID);
        v2 BitmapDimInMeters = PixelsToMeters*V2i(BitmapInfo->Dim[0], BitmapInfo->Dim[1]);

        PushBitmap(RenderGroup, BitmapID, BitmapDimInMeters.y, V3(-D, 0) - V3(0.5f, 0.5f, 0));
    }
}



