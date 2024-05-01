/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */
#if 0
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
                r32 TileSideInMeters, layout *Layout)
{
    object_transform Transform = DefaultUprightTransform();
    if(GameState->AllowEdit)
    {
        if(Input->MouseButtons[0].EndedDown)
        {
            ChangeTile(RenderGroup, GameState, MouseChunkP);
        }

        ssa_tileset *Info = GetTilesetInfo(RenderGroup->Assets, TilesetID);
//        ChangeCursorPositionFor(&GameState->TileMenuBarCursor, Info->TileCount, Input->MouseZ);
//        ShowTest(TextRenderGroup, &GameState->TileMenuBarCursor);
        ShowTileMenuBar(RenderGroup, &GameState->TileMenuBarCursor, TileSideInMeters);
//        ShowTilesetStats(TextRenderGroup, GameState);
    
        WriteMap("tilemap.bin", GameState->WorldTileCount, GameState->TileIDs);
        WriteWorldTiles("worldtiles.bin", GameState->WorldTileCount, GameState->WorldTiles);
        ReloadTileset(RenderGroup->Assets, GameState);
    }
}
#endif

internal void
UpdateAndRenderTerrainMode()
{
    
#if 0    
    if(Input->ExecutableReloaded)
    {
        ResetGroundBuffers(TranState, 0);
    }

    world *World = EditorState->World;

    game_controller_input *Controller = GetController(Input, 0);
    if(Controller->IsAnalog)
    {
        // NOTE(casey): Use analog movement tuning
    }
    else
    {
        v2 NewP = {};
        // NOTE(casey): Use digital movement tuning
        if(Controller->MoveUp.EndedDown)
        {
            NewP.y += 4.0f;
        }
        if(Controller->MoveDown.EndedDown)
        {
            NewP.y -= 4.0f;
        }
        if(Controller->MoveLeft.EndedDown)
        {
            NewP.x -= 4.0f;
        }
        if(Controller->MoveRight.EndedDown)
        {
            NewP.x += 4.0f;
        }
        
        EditorState->CameraP = MapIntoChunkSpace(World, EditorState->CameraP, NewP);

        if(Controller->ChangeEditMode.EndedDown)
        {
            EditorState->EditMode += 1;
            if(EditorState->EditMode >= EditMode_Count)
            {
                EditorState->EditMode = 0;
            }
        }

        if(WasPressed(Controller->RightShoulder))
        {
            EditorState->AllowEdit = !EditorState->AllowEdit;
        }
        
        if(EditorState->EditMode == EditMode_Terrain)
        {
            if(Controller->Biome.EndedDown)
            {
                EditorState->TileSetStats.Biome += 1;
                if(EditorState->TileSetStats.Biome >= BiomeType_Count)
                {
                    EditorState->TileSetStats.Biome = 0;
                }
            }

            if(Controller->Type.EndedDown)
            {
                EditorState->TileSetStats.Type += 1;
                if(EditorState->TileSetStats.Type >= TileType_Count)
                {
                    EditorState->TileSetStats.Type = 0;
                }
            }

            if(Controller->Height.EndedDown)
            {
                EditorState->TileSetStats.Height += 1;
                if(EditorState->TileSetStats.Height >= Height_Count)
                {
                    EditorState->TileSetStats.Height = 0;
                }
            }

            if(Controller->CliffHillType.EndedDown)
            {
                EditorState->TileSetStats.CliffHillType += 1;
                if(EditorState->TileSetStats.CliffHillType >= CliffHillType_Count)
                {
                    EditorState->TileSetStats.CliffHillType = 0;
                }
            }

            if(Controller->MainSurface.EndedDown)
            {
                EditorState->TileSetStats.MainSurface += 1;
                if(EditorState->TileSetStats.MainSurface >= TileSurface_Count)
                {
                    EditorState->TileSetStats.MainSurface = 0;
                }
            }
            if(Controller->MergeSurface.EndedDown)
            {
                EditorState->TileSetStats.MergeSurface += 1;
                if(EditorState->TileSetStats.MergeSurface >= TileSurface_Count)
                {
                    EditorState->TileSetStats.MergeSurface = 0;
                }
            }
        }
        else if(EditorState->EditMode == EditMode_Decoration)
        {
            if(Controller->Biome.EndedDown)
            {
                EditorState->AssetSetStats.Biome += 1;
                if(EditorState->AssetSetStats.Biome >= BiomeType_Count)
                {
                    EditorState->AssetSetStats.Biome = 0;
                }
            }

            if(Controller->Type.EndedDown)
            {
                EditorState->AssetSetStats.Type += 1;
                if(EditorState->AssetSetStats.Type >= Asset_Count)
                {
                    EditorState->AssetSetStats.Type = 0;
                }
            }
        }

        if(Controller->ActionUp.EndedDown)
        {
//            ChangeChoosenAttributeValueFor(EditorState, true);
        }
        if(Controller->ActionDown.EndedDown)
        {
//            ChangeChoosenAttributeValueFor(EditorState, false);
        }
    }

    // TODO(paul): Initialize own render group for each of the edit modes
    render_group *RenderGroup = &RenderGroup_;
    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)DrawBuffer.Width*WidthOfMonitor;// / 2.0f;
    real32 FocalLength = 0.5f;
    real32 DistanceAboveTarget = 10.0f;
    Perspective(RenderGroup, DrawBuffer.Width, DrawBuffer.Height, MetersToPixels, FocalLength, DistanceAboveTarget);

    // NOTE(paul): Load GlobalTileset
    GlobalTileset = PushTileset(RenderGroup, EditorState->GlobalTilesetID, true);
    GlobalTilesetInfo = GetTilesetInfo(TranState->Assets, EditorState->GlobalTilesetID);
    if(!EditorState->WorldTilesInitialized)
    {
        // TODO(paul): Split this into three functions
        InitializeWorldTilesAndDecorations(RenderGroup, TranState->Assets, EditorState,
                                           "worldtiles.bin", "decorations.bin", "collisions.bin");
    }

    // TODO(paul): All of this below 
//    Clear(RenderGroup, V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
//    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

    v2 ScreenCenter = {0.5f*(real32)DrawBuffer.Width,
                       0.5f*(real32)DrawBuffer.Height};

    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
    rectangle2 CameraBoundsInMeters = RectMinMax(ScreenBounds.Min, ScreenBounds.Max);

    v2 SimBoundsExpansion = {15.0f, 14.0f};
    rectangle2 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);

    object_transform Transform = DefaultUprightTransform();    

    r32 MouseX = (r32)Input->MouseX;
    r32 MouseY = (r32)Input->MouseY;
    v2 P = Unproject(RenderGroup, Transform, V2(MouseX, MouseY)).xy;

    world_position MouseChunkP = MapIntoChunkSpace(World, EditorState->CameraP, P);
    tile_position MouseTileP = TilePositionFromChunkPosition(&MouseChunkP);

    tile_position Tp = TilePositionFromChunkPosition(&MouseChunkP);
    tile_position TCp = TilePositionFromChunkPosition(&EditorState->CameraP);

    v2 dTile =
        {
            (real32)TCp.TileX - (real32)Tp.TileX,
            (real32)TCp.TileY - (real32)Tp.TileY
        };

    v2 D = dTile*TileSideInMeters - V2(0.5f, 0.5f);

    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "Edit Mode: %s",
                EditModeText[EditorState->EditMode]);
    DEBUGTextLine(&TextRenderGroup, TextBuffer);
    
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "CameraP in Chunks: X: %d Y: %d, OX: %f, OY: %f",
                EditorState->CameraP.ChunkX, EditorState->CameraP.ChunkY,
                EditorState->CameraP.Offset_.x, EditorState->CameraP.Offset_.y);
    DEBUGTextLine(&TextRenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "MouseP in Chunks: X: %d Y: %d, OX: %f, OY: %f",
                MouseChunkP.ChunkX, MouseChunkP.ChunkY,
                MouseChunkP.Offset_.x, MouseChunkP.Offset_.y);
    DEBUGTextLine(&TextRenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "MouseP in Tiles: X: %d Y: %d",
                MouseTileP.TileX, MouseTileP.TileY);
    DEBUGTextLine(&TextRenderGroup, TextBuffer);

    if(EditorState->RenderGround)
    {
        // NOTE(casey): Ground chunk updating
        UpdateGroundChunks(EditorState, TranState, World, CameraBoundsInMeters, TileSideInMeters);

        // NOTE(casey): Ground chunk rendering
        RenderGroundChunks(EditorState, TranState, World, RenderGroup, Transform);
    }

    Transform.SortBias = 0.0f;

    if(EditorState->RenderDecorations)
    {
        // NOTE(paul): Render decorations
        RenderDecorations(EditorState, SimBounds, RenderGroup, Transform);
    }

    BeginRow(&Layout);
    ActionButton(&Layout, "Collision", SetUInt32Interaction(CID, (u32 *)&EditorState->EditMode, EditMode_Collision));
    ActionButton(&Layout, "Decoration", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Decoration));
    ActionButton(&Layout, "Assets", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Assets));
    EndRow(&Layout);

    BeginRow(&Layout);
    BooleanButton(&Layout, "RenderGround", EditorState->RenderGround,
                  SetUInt32Interaction(CID, (u32 *)&EditorState->RenderGround, !EditorState->RenderGround));
    BooleanButton(&Layout, "RenderDecorations", EditorState->RenderDecorations,
                  SetUInt32Interaction(CID, (u32 *)&EditorState->RenderDecorations, !EditorState->RenderDecorations));
    BooleanButton(&Layout, "AllowEdit", EditorState->AllowEdit,
                  SetUInt32Interaction(DID, (u32 *)&EditorState->AllowEdit, !EditorState->AllowEdit));
    BooleanButton(&Layout, "ShowBorders", EditorState->ShowBorders,
                  SetUInt32Interaction(DID, (u32 *)&EditorState->ShowBorders, !EditorState->ShowBorders));
    EndRow(&Layout);

    TerrainEditMode(RenderGroup, EditorState, TranState, Input, &MouseChunkP, TileSideInMeters, &Layout);
    ShowTileCursor(EditorState, RenderGroup, Transform, MouseChunkP, TileSideInMeters, D);

    if(EditorState->ShowBorders)
    {
        PushRectOutline(RenderGroup, Transform, V3(0, 0, 0), GetDim(ScreenBounds), V4(1.0f, 1.0f, 0.0f, 1));
        PushRectOutline(RenderGroup, Transform, V3(0, 0, 0), GetDim(SimBounds), V4(1.0f, 0.0f, 1.0f, 1));
    }

    // NOTE(paul): Advance Time
    EditorState->Time += Input->dtForFrame;
#endif
}
