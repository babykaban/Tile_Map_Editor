/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */
#include "editor_file_builder.cpp"

internal void
UpdateAndRenderAssetsMode(editor_state *EditorState, transient_state *TranState,
                          render_group *RenderGroup, game_input *Input, edit_mode_asset *AssetMode,
                          game_render_commands *RenderCommands)
{
    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)RenderCommands->Width*WidthOfMonitor;// / 2.0f;
    real32 FocalLength = 0.5f;
    real32 DistanceAboveTarget = 10.0f;
    Perspective(RenderGroup, RenderCommands->Width, RenderCommands->Height, MetersToPixels, FocalLength, DistanceAboveTarget);

    builder_assets *BuilderAssets = &AssetMode->BuilderAssets;
    
    switch(AssetMode->AssetAddMode)
    {
        case AssetMode_Bitmap:
        {
//            AssetAddModeBitmap(AssetMode, UIState, &Layout, RenderGroup);
        } break;

        case AssetMode_Sound:
        {
//            AssetAddModeSound(AssetMode, UIState, &Layout);
        } break;

        case AssetMode_AssetSet:
        {
//            AssetAddModeAssetSet(AssetMode, UIState, &Layout);
        } break;

        case AssetMode_SpriteSheet:
        {
//            AssetAddModeSpriteSheet(AssetMode, UIState, &Layout);
        } break;

        case AssetMode_TileSet:
        {
//            AssetAddModeTileSet(AssetMode, UIState, &Layout);
        } break;

        case AssetMode_Font:
        {
//            AssetAddModeFont(AssetMode, UIState, &Layout);
        } break;

        case AssetMode_Quest:
        {
//            AssetAddModeQuest(AssetMode, UIState, &Layout);
        } break;

        case AssetMode_BinaryFile:
        {
//            AssetAddModeBinaryFile(AssetMode, UIState, &Layout);
        } break;

        InvalidDefaultCase;
    }
}
