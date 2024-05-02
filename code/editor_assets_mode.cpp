/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */
#include "editor_file_builder.cpp"

internal void
AssetAddModeBitmap(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout, render_group *RenderGroup)
{
    BeginRow(Layout);
    ActionButton(Layout, "Sound", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Sound));
    ActionButton(Layout, "Assetset", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                          (u32 *)&AssetMode->AssetAddMode, AssetMode_AssetSet));
    ActionButton(Layout, "SpriteSheet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_SpriteSheet));
    ActionButton(Layout, "TileSet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                         (u32 *)&AssetMode->AssetAddMode, AssetMode_TileSet));
    ActionButton(Layout, "Font", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                      (u32 *)&AssetMode->AssetAddMode, AssetMode_Font));
    ActionButton(Layout, "Quest", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Quest));
    ActionButton(Layout, "BitnaryFile", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_BinaryFile));
    EndRow(Layout);

    layout GridLayout = BeginLayout(UIContext, Layout->MouseP, V2(-10.0f, 530.0f));
    ui_interaction NullInteraction = {};
    rectangle2 Rect = Grid(&GridLayout, V2(960.0f, 960.0f), V2(64.0f, 64.0f), NullInteraction, V4(0, 0, 0, 1));
    EndLayout(&GridLayout);
    
    u32 Result = SimpleScrollElement(Layout, &AssetMode->TestCursor,
                                     AdvanceArrayCursorInteraction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                                   &AssetMode->TestCursor, AssetMode->BMPFileCount));

    char *FileName = AssetMode->BMPFileNames[Result];
    char Buffer[512];
    sprintf_s(Buffer, "%s/%s", "editor/bmps", FileName);

    // TODO(paul): Free Bitmap memory
    loaded_bitmap Bitmap = LoadBMP(Buffer);
    Bitmap.AlignPercentage = V2(0.5f, 0.5f);
    
    if(Bitmap.TextureHandle)
    {
        Platform.DeallocateTexture(Bitmap.TextureHandle);
        Bitmap.TextureHandle = 
            Platform.AllocateTexture(Bitmap.Width, Bitmap.Height, Bitmap.Memory);
    }
    else
    {
        Bitmap.TextureHandle = 
            Platform.AllocateTexture(Bitmap.Width, Bitmap.Height, Bitmap.Memory);
    }
    
    PushBitmap(&UIContext->RenderGroup, UIContext->UITransform, &Bitmap, (r32)(2.0f*Bitmap.Height), V3(GetCenter(Rect), 0));

    PushLine(&UIContext->RenderGroup, UIContext->UITransform, V3(0, 0, 0), V3(Layout->MouseP, 0), V4(1, 0, 0, 1));
}

internal void
AssetAddModeSound(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout)
{
    BeginRow(Layout);
    ActionButton(Layout, "Bitmap", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                        (u32 *)&AssetMode->AssetAddMode, AssetMode_Bitmap));
    ActionButton(Layout, "Assetset", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                          (u32 *)&AssetMode->AssetAddMode, AssetMode_AssetSet));
    ActionButton(Layout, "SpriteSheet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_SpriteSheet));
    ActionButton(Layout, "TileSet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                         (u32 *)&AssetMode->AssetAddMode, AssetMode_TileSet));
    ActionButton(Layout, "Font", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                      (u32 *)&AssetMode->AssetAddMode, AssetMode_Font));
    ActionButton(Layout, "Quest", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Quest));
    ActionButton(Layout, "BitnaryFile", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_BinaryFile));
    EndRow(Layout);
}

internal void
AssetAddModeAssetSet(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout)
{
    BeginRow(Layout);
    ActionButton(Layout, "Bitmap", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                        (u32 *)&AssetMode->AssetAddMode, AssetMode_Bitmap));
    ActionButton(Layout, "Sound", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Sound));
    ActionButton(Layout, "SpriteSheet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_SpriteSheet));
    ActionButton(Layout, "TileSet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                         (u32 *)&AssetMode->AssetAddMode, AssetMode_TileSet));
    ActionButton(Layout, "Font", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                      (u32 *)&AssetMode->AssetAddMode, AssetMode_Font));
    ActionButton(Layout, "Quest", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Quest));
    ActionButton(Layout, "BitnaryFile", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_BinaryFile));
    EndRow(Layout);
}

internal void
AssetAddModeSpriteSheet(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout)
{
    BeginRow(Layout);
    ActionButton(Layout, "Bitmap", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                        (u32 *)&AssetMode->AssetAddMode, AssetMode_Bitmap));
    ActionButton(Layout, "Sound", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Sound));
    ActionButton(Layout, "Assetset", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                          (u32 *)&AssetMode->AssetAddMode, AssetMode_AssetSet));
    ActionButton(Layout, "TileSet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                         (u32 *)&AssetMode->AssetAddMode, AssetMode_TileSet));
    ActionButton(Layout, "Font", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                      (u32 *)&AssetMode->AssetAddMode, AssetMode_Font));
    ActionButton(Layout, "Quest", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Quest));
    ActionButton(Layout, "BitnaryFile", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_BinaryFile));
    EndRow(Layout);
}

internal void
AssetAddModeTileSet(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout)
{
    BeginRow(Layout);
    ActionButton(Layout, "Bitmap", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                        (u32 *)&AssetMode->AssetAddMode, AssetMode_Bitmap));
    ActionButton(Layout, "Sound", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Sound));
    ActionButton(Layout, "Assetset", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                          (u32 *)&AssetMode->AssetAddMode, AssetMode_AssetSet));
    ActionButton(Layout, "SpriteSheet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_SpriteSheet));
    ActionButton(Layout, "Font", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                      (u32 *)&AssetMode->AssetAddMode, AssetMode_Font));
    ActionButton(Layout, "Quest", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Quest));
    ActionButton(Layout, "BitnaryFile", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_BinaryFile));
    EndRow(Layout);
}

internal void
AssetAddModeFont(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout)
{
    BeginRow(Layout);
    ActionButton(Layout, "Bitmap", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                        (u32 *)&AssetMode->AssetAddMode, AssetMode_Bitmap));
    ActionButton(Layout, "Sound", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Sound));
    ActionButton(Layout, "Assetset", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                          (u32 *)&AssetMode->AssetAddMode, AssetMode_AssetSet));
    ActionButton(Layout, "SpriteSheet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_SpriteSheet));
    ActionButton(Layout, "TileSet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                         (u32 *)&AssetMode->AssetAddMode, AssetMode_TileSet));
    ActionButton(Layout, "Quest", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Quest));
    ActionButton(Layout, "BitnaryFile", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_BinaryFile));
    EndRow(Layout);
}

internal void
AssetAddModeQuest(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout)
{
    BeginRow(Layout);
    ActionButton(Layout, "Bitmap", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                        (u32 *)&AssetMode->AssetAddMode, AssetMode_Bitmap));
    ActionButton(Layout, "Sound", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Sound));
    ActionButton(Layout, "Assetset", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                          (u32 *)&AssetMode->AssetAddMode, AssetMode_AssetSet));
    ActionButton(Layout, "SpriteSheet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_SpriteSheet));
    ActionButton(Layout, "TileSet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                         (u32 *)&AssetMode->AssetAddMode, AssetMode_TileSet));
    ActionButton(Layout, "Font", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                      (u32 *)&AssetMode->AssetAddMode, AssetMode_Font));
    ActionButton(Layout, "BitnaryFile", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_BinaryFile));
    EndRow(Layout);
}

internal void
AssetAddModeBinaryFile(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout)
{
    BeginRow(Layout);
    ActionButton(Layout, "Bitmap", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                        (u32 *)&AssetMode->AssetAddMode, AssetMode_Bitmap));
    ActionButton(Layout, "Sound", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Sound));
    ActionButton(Layout, "Assetset", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                          (u32 *)&AssetMode->AssetAddMode, AssetMode_AssetSet));
    ActionButton(Layout, "SpriteSheet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                             (u32 *)&AssetMode->AssetAddMode, AssetMode_SpriteSheet));
    ActionButton(Layout, "TileSet", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                         (u32 *)&AssetMode->AssetAddMode, AssetMode_TileSet));
    ActionButton(Layout, "Font", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                      (u32 *)&AssetMode->AssetAddMode, AssetMode_Font));
    ActionButton(Layout, "Quest", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                       (u32 *)&AssetMode->AssetAddMode, AssetMode_Quest));
    EndRow(Layout);
}

internal void
UpdateAndRenderAssetsMode(editor_state *EditorState, transient_state *TranState, ui_context *UIContext,
                          render_group *RenderGroup, game_input *Input, edit_mode_asset *AssetMode,
                          game_render_commands *RenderCommands)
{
    v2 MouseP = Unproject(&UIContext->RenderGroup, DefaultFlatTransform(), V2i(Input->MouseX, Input->MouseY)).xy;
    layout Layout = BeginLayout(UIContext, MouseP, V2(UIContext->LeftEdge, 0.5f*UIContext->Height));

    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)RenderCommands->Width*WidthOfMonitor;// / 2.0f;
    real32 FocalLength = 0.5f;
    real32 DistanceAboveTarget = 10.0f;
    Perspective(RenderGroup, RenderCommands->Width, RenderCommands->Height, MetersToPixels, FocalLength, DistanceAboveTarget);
    
#if 0
#endif
    switch(AssetMode->AssetAddMode)
    {
        case AssetMode_Bitmap:
        {
            AssetAddModeBitmap(AssetMode, UIContext, &Layout, RenderGroup);
        } break;

        case AssetMode_Sound:
        {
            AssetAddModeSound(AssetMode, UIContext, &Layout);
        } break;

        case AssetMode_AssetSet:
        {
            AssetAddModeAssetSet(AssetMode, UIContext, &Layout);
        } break;

        case AssetMode_SpriteSheet:
        {
            AssetAddModeSpriteSheet(AssetMode, UIContext, &Layout);
        } break;

        case AssetMode_TileSet:
        {
            AssetAddModeTileSet(AssetMode, UIContext, &Layout);
        } break;

        case AssetMode_Font:
        {
            AssetAddModeFont(AssetMode, UIContext, &Layout);
        } break;

        case AssetMode_Quest:
        {
            AssetAddModeQuest(AssetMode, UIContext, &Layout);
        } break;

        case AssetMode_BinaryFile:
        {
            AssetAddModeBinaryFile(AssetMode, UIContext, &Layout);
        } break;

        InvalidDefaultCase;
    }
}
