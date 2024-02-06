/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

internal void
WriteCollisions(char *FileName, u32 Count, collision *Collisions)
{
    uint32 ContentSize = sizeof(collision)*Count;
    Platform.DEBUGWriteEntireFile(0, FileName, ContentSize, Collisions);
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
CollisionEditMode(render_group *RenderGroup, render_group *TextRenderGroup, game_state *GameState,
                  transient_state *TranState, game_input *Input, world_position *MouseChunkP,
                  r32 TileSideInMeters)
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
