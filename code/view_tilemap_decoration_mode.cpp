/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

internal void
DecorationEditMode(render_group *RenderGroup, render_group *TextRenderGroup, game_state *GameState,
                   transient_state *TranState, game_input *Input, world_position *MouseChunkP,
                   r32 TileSideInMeters)
{

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
                    V4(0.0f, 0.0f, 1.0f, 1), 0.0125f);

    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    WeightVector.E[Tag_AssetType] = 1.0f;

    MatchVector.E[Tag_AssetType] = (r32)Asset_Tree;
    assetset_id ID = GetBestMatchAssetsetFrom(TranState->Assets, Asset_AssetSet,
                                              &MatchVector, &WeightVector);

    loaded_assetset *Assetset = PushAssetset(RenderGroup, ID, true);
    ssa_assetset *AssetsetInfo = GetAssetsetInfo(TranState->Assets, ID);
    bitmap_id BitmapID = GetBitmapFromAssetset(TranState->Assets, AssetsetInfo, Assetset, 26);
    PushBitmap(RenderGroup, BitmapID, 4.0f, V3(-D, 0));
}
