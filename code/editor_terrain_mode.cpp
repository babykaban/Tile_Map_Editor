/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */
global_variable tileset_id TilesetID; 
global_variable loaded_tileset *GlobalTileset; 
global_variable ssa_tileset *GlobalTilesetInfo;

internal void
WriteMap(char *FileName, u32 TileCount, u32 *TileIDs)
{
    uint32 ContentSize = sizeof(u32)*TileCount;
    Platform.DEBUGWriteEntireFile(FileName, ContentSize, TileIDs);
}

internal void
WriteWorldTiles(char *FileName, u32 TileCount, world_tile *WorldTiles)
{
    uint32 ContentSize = sizeof(world_tile)*TileCount;
    Platform.DEBUGWriteEntireFile(FileName, ContentSize, WorldTiles);
}

inline tile_position
TilePositionFromChunkPosition(world_position *Pos)
{
    tile_position Result = {};

    u32 HalfChunkCount = TILES_PER_CHUNK / 2;
    Result.TileX = Pos->ChunkX*TILES_PER_CHUNK + HalfChunkCount;
    Result.TileY = Pos->ChunkY*TILES_PER_CHUNK + HalfChunkCount; 

    if(Pos->Offset_.x > 0.0f)
    {
        Result.TileX += CeilReal32ToInt32(Pos->Offset_.x) - 1;
    }
    else
    {
        Result.TileX += FloorReal32ToInt32(Pos->Offset_.x);
    }

    if(Pos->Offset_.y > 0.0f)
    {
        Result.TileY += CeilReal32ToInt32(Pos->Offset_.y) - 1;
    }
    else
    {
        Result.TileY += FloorReal32ToInt32(Pos->Offset_.y);
    }

    return(Result);
}

internal u32
GetTileIndexFromChunkPosition(game_state *GameState, world_position *MouseP)
{
    u32 Result = 0;
    tile_position MouseTileP = TilePositionFromChunkPosition(MouseP);
    u32 WorldTileIndex = MouseTileP.TileY*WORLD_HEIGHT_TILE_COUNT + MouseTileP.TileX;

    Result = WorldTileIndex;
    
    return(Result);
}


// TODO(paul): Reset only groundbuffers that was changed
inline void
ResetGroundBuffers(transient_state *TranState, world_position *ChunkP)
{
    if(ChunkP)
    {
        for(uint32 GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            if(IsValid(GroundBuffer->P) &&
               (GroundBuffer->P.ChunkX == ChunkP->ChunkX) &&
               (GroundBuffer->P.ChunkY == ChunkP->ChunkY))
            {
                GroundBuffer->P = NullPosition();            
                break;
            }
        }        
    }
    else
    {
        for(uint32 GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->P = NullPosition();            
        }        
    }
}

internal void
ChangeTile(render_group *RenderGroup, game_state *GameState, world_position *MouseP)
{
    u32 TileToChange = GetTileIndexFromChunkPosition(GameState, MouseP);
    if((MouseP->ChunkX >= 0) && (MouseP->ChunkY >= 0) && (TileToChange < GameState->WorldTileCount))
    {
        loaded_tileset *Tileset = PushTileset(RenderGroup, TilesetID);
        if(Tileset)
        {
            u32 TileIndex = GameState->TileMenuBarCursor.Array[GameState->TileMenuBarCursor.ArrayPosition];
            ssa_tile *Tile = Tileset->Tiles + TileIndex; 
            GameState->WorldTiles[TileToChange].TileID = Tile->UniqueID;
            GameState->WorldTiles[TileToChange].TileBitmapID.Value = Tile->BitmapID.Value + Tileset->BitmapIDOffset;

            GameState->TileIDs[TileToChange] = Tile->UniqueID;

            ResetGroundBuffers(RenderGroup->Assets->TranState, MouseP);
        }
    }
}

internal void
ShowTest(render_group *RenderGroup, array_cursor *Cursor)
{
    DEBUGTextLine(RenderGroup, "Test:");
    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
                Cursor->Array[0], Cursor->Array[1], Cursor->Array[2], Cursor->Array[3],
                Cursor->Array[4], Cursor->Array[5], Cursor->Array[6], Cursor->Array[7],
                Cursor->Array[8], Cursor->Array[9]);
    DEBUGTextLine(RenderGroup, TextBuffer);
}

internal void
ShowTileMenuBar(render_group *RenderGroup, array_cursor *Cursor, r32 TileDimInMeters)
{
    object_transform Transform = DefaultUprightTransform();
    Transform.SortBias = 100.0f;
    loaded_tileset *Tileset = PushTileset(RenderGroup, TilesetID);
    u32 TileCountInBar = Cursor->ArrayCount;

    r32 TileHalfDim = 0.5f*TileDimInMeters;
    r32 MenuBarWidth = TileDimInMeters*TileCountInBar;
    r32 HalfMenuBarWidth = 0.5f*MenuBarWidth;
    r32 OffsetY = 8.0f;
    PushRectOutline(RenderGroup, Transform, V3(0, OffsetY, 0), V2(MenuBarWidth, TileDimInMeters), V4(0, 1, 0, 1), 0.05f);

    ssa_tileset *Info = GetTilesetInfo(RenderGroup->Assets, TilesetID);
    if(Tileset)
    {
        r32 TileOffsetX = -HalfMenuBarWidth;
        r32 TileOffsetY = OffsetY - TileHalfDim;
        for(u32 Index = 0;
            Index < TileCountInBar;
            ++Index)
        {
            u32 TileIndex = Cursor->Array[Index];
            bitmap_id ID = GetBitmapForTile(RenderGroup->Assets, Info, Tileset, TileIndex);
            PushBitmap(RenderGroup, Transform, ID, TileDimInMeters, V3(TileOffsetX, TileOffsetY, 0));
            TileOffsetX += TileDimInMeters;
        }
    }

    r32 CursorOffsetX = (TileHalfDim - HalfMenuBarWidth) + Cursor->ArrayPosition*TileDimInMeters;
    Transform.SortBias = 1000.0f;
    PushRectOutline(RenderGroup, Transform, V3(CursorOffsetX, OffsetY, 0), V2(TileDimInMeters, TileDimInMeters), V4(1, 1, 1, 1), 0.05f);
}

internal void
ShowTilesetStats(render_group *RenderGroup, game_state *GameState)
{
    DEBUGTextLine(RenderGroup, "Tileset Stats:");
    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer), "Biome: %s", Biomes[GameState->TileSetStats.Biome]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "TileType: %s", TileTypes[GameState->TileSetStats.Type]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "Height: %s", Heights[GameState->TileSetStats.Height]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "CliffHillType: %s",
                CliffHillTypes[GameState->TileSetStats.CliffHillType]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "MainSurface: %s", Surfaces[GameState->TileSetStats.MainSurface]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "MergeSurface: %s", Surfaces[GameState->TileSetStats.MergeSurface]);
    DEBUGTextLine(RenderGroup, TextBuffer);
}

internal void
ReloadTileset(game_assets *Assets, game_state *GameState)
{
    asset_vector WeightVector = {};
    WeightVector.E[Tag_BiomeType] = 1.0f;
    WeightVector.E[Tag_TileType] = 1.0f;
    WeightVector.E[Tag_Height] = 1.0f;
    WeightVector.E[Tag_CliffHillType] = 1.0f;
    WeightVector.E[Tag_TileMainSurface] = 1.0f;
    WeightVector.E[Tag_TileMergeSurface] = 1.0f;

    asset_vector MatchVector = {};
    MatchVector.E[Tag_BiomeType] = (r32)GameState->TileSetStats.Biome;
    MatchVector.E[Tag_TileType] = (r32)GameState->TileSetStats.Type;
    MatchVector.E[Tag_Height] = (r32)GameState->TileSetStats.Height;
    MatchVector.E[Tag_CliffHillType] = (r32)GameState->TileSetStats.CliffHillType;
    MatchVector.E[Tag_TileMainSurface] = (r32)GameState->TileSetStats.MainSurface;
    MatchVector.E[Tag_TileMergeSurface] = (r32)GameState->TileSetStats.MergeSurface;
    
    tileset_id ID = GetBestMatchTilesetFrom(Assets, Asset_Tileset, &MatchVector, &WeightVector);
    if(ID.Value != TilesetID.Value)
    {
        TilesetID = ID;
        ResetCursorArray(&GameState->TileMenuBarCursor);
    }
}

internal void
TerrainEditMode(render_group *RenderGroup, game_state *GameState,
                transient_state *TranState, game_input *Input, world_position *MouseChunkP,
                r32 TileSideInMeters)
{
    object_transform Transform = DefaultUprightTransform();
    if(GameState->AllowEdit)
    {
        if(Input->MouseButtons[0].EndedDown)
        {
            ChangeTile(RenderGroup, GameState, MouseChunkP);
        }

        ssa_tileset *Info = GetTilesetInfo(RenderGroup->Assets, TilesetID);
        ChangeCursorPositionFor(&GameState->TileMenuBarCursor, Info->TileCount, Input->MouseZ);
//        ShowTest(TextRenderGroup, &GameState->TileMenuBarCursor);
        ShowTileMenuBar(RenderGroup, &GameState->TileMenuBarCursor, TileSideInMeters);
//        ShowTilesetStats(TextRenderGroup, GameState);
    
        WriteMap("tilemap.bin", GameState->WorldTileCount, GameState->TileIDs);
        WriteWorldTiles("worldtiles.bin", GameState->WorldTileCount, GameState->WorldTiles);
        ReloadTileset(RenderGroup->Assets, GameState);
    }
}
