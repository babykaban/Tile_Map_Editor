/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */
#if 0
internal void
WriteCollisions(char *FileName, u32 Count, collision *Collisions)
{
    uint32 ContentSize = sizeof(collision)*Count;
    Platform.DEBUGWriteEntireFile(FileName, ContentSize, Collisions);
}

internal void
AddCollision(render_group *RenderGroup, game_state *GameState, world_position *MouseP, r32 TileSideInMeters)
{
    u32 CollisionIndex = GetTileIndexFromChunkPosition(GameState, MouseP);
    if((MouseP->ChunkX >= 0) && (MouseP->ChunkY >= 0))
    {
        GameState->Collisions[CollisionIndex].Rect = RectMinDim(V2(0, 0), V2(TileSideInMeters, TileSideInMeters));

        tile_position TileP = TilePositionFromChunkPosition(MouseP);
        GameState->Collisions[CollisionIndex].P =
            ChunkPositionFromTilePosition(GameState->World, TileP.TileX, TileP.TileY);
    }
}

internal void
RemoveCollision(render_group *RenderGroup, game_state *GameState, world_position *MouseP)
{
    u32 CollisionIndex = GetTileIndexFromChunkPosition(GameState, MouseP);
    if((MouseP->ChunkX >= 0) && (MouseP->ChunkY >= 0))
    {
        GameState->Collisions[CollisionIndex].Rect = InvertedInfinityRectangle2();
        GameState->Collisions[CollisionIndex].P = {};
    }
}

internal void
CollisionEditMode(render_group *RenderGroup, game_state *GameState,
                  transient_state *TranState, game_input *Input, world_position *MouseChunkP,
                  r32 TileSideInMeters)
{
    if(GameState->AllowEdit)
    {
        if(Input->MouseButtons[0].EndedDown)
        {
            AddCollision(RenderGroup, GameState, MouseChunkP, TileSideInMeters);
            WriteCollisions("collisions.bin", GameState->WorldTileCount, GameState->Collisions);
        }

        if(Input->MouseButtons[2].EndedDown)
        {
            RemoveCollision(RenderGroup, GameState, MouseChunkP);
            WriteCollisions("collisions.bin", GameState->WorldTileCount, GameState->Collisions);
        }
    }
}
#endif

internal void
UpdateAndRenderCollisionMode()
{
#if 0
    BeginRow(&Layout);
    ActionButton(&Layout, "Terrain", SetUInt32Interaction(TID, (u32 *)&EditorState->EditMode, EditMode_Terrain));
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

    // NOTE(paul): Render collisions
    for(u32 CollisionIndex = 0;
        CollisionIndex < EditorState->WorldTileCount;
        ++CollisionIndex)
    {
        collision *Collision = EditorState->Collisions + CollisionIndex;
        if(IsValid(Collision->P))
        {
            v2 Delta = Subtract(EditorState->World, &Collision->P, &EditorState->CameraP) - V2(2.0f, 2.0f);
            if(IsInRectangle(SimBounds, Delta))
            {
                Transform.OffsetP = V3(Delta, 0);
                PushRectOutline(RenderGroup, Transform, Collision->Rect, 0.0f, V4(0, 0, 1, 1), 0.025f);
            }
        }
    }

    CollisionEditMode(RenderGroup, EditorState, TranState, Input, &MouseChunkP,
                      TileSideInMeters);
    ShowTileCursor(EditorState, RenderGroup, Transform, MouseChunkP, TileSideInMeters, D);
#endif
}
