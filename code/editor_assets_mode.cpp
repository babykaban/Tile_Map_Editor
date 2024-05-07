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

    layout GridLayout = BeginLayout(UIContext, Layout->MouseP, V2(0.0f, 0.0f));
    ui_interaction NullInteraction = {};
//    rectangle2 Rect = Grid(&GridLayout, V2(960.0f, 960.0f), V2(64.0f, 64.0f), NullInteraction, V4(0, 0, 0, 0.2f));
    EndLayout(&GridLayout);
    
    if(AssetMode->BMPFileCount)
    {
        BeginRow(Layout);
        u32 Result = SimpleScrollElement(Layout, &AssetMode->BMPCursor,
                                         AdvanceArrayCursorInteraction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                                       &AssetMode->BMPCursor, AssetMode->BMPFileCount),
                                         V4(0.8f, 0.8f, 0.8f, 1), V4(1, 1, 1, 1), 4.0f, V4(0.0f, 0.635294117647f, 0.909803921569f, 1));

        ActionButton(Layout, "AddAsset", SetUInt32Interaction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                              (u32 *)&AssetMode->AddAsset, true));
        EndRow(Layout);

        char *FileName = AssetMode->BMPFileNames[Result];
        char Buffer[512];
        sprintf_s(Buffer, "%s/%s", "editor/bmps", FileName);

        if(!(AssetMode->AddBitmap.Memory) || (AssetMode->BitmapIndex != Result))
        {
            if(AssetMode->AddBitmap.TextureHandle)
            {
                Platform.DeallocateTexture(AssetMode->AddBitmap.TextureHandle);
                Platform.FreeFileMemory(AssetMode->AddBitmap.Memory);
            }

            AssetMode->BitmapIndex = Result;
            AssetMode->AddBitmap = LoadBMP(Buffer);
        }
    
        ui_item_id BitmapItemID = UIItemIDFromEditMode(UIContext, EditMode_Assets);
        ui_interaction BitmapInteraction =
            SetUInt32Interaction(BitmapItemID, (u32 *)&AssetMode->PlaceAlignment, true);
        
        ui_view *View = GetOrCreateDebugViewFor(UIContext, BitmapItemID);
        r32 BitmapScale = View->InlineBlock.Dim.y;
    
        BeginRow(Layout);

        r32 PixelDim = 0.0f;
        if(BitmapScale)
        {
            PixelDim = (BitmapScale / AssetMode->AddBitmap.Height);
        }

        rectangle2 BitmapBounds = UIBitmap(Layout, &AssetMode->AddBitmap, View, BitmapScale,
                                           V2(32.0f, 32.0f), BitmapInteraction);
        v2 BitmapScaledDim = GetDim(BitmapBounds);
        
        // TODO(paul): Fix align offset
        layout BitmapLayout = BeginLayout(UIContext, Layout->MouseP, Layout->At);
        sprintf_s(Buffer, "Width: %d", AssetMode->AddBitmap.Width);
        Label(&BitmapLayout, Buffer);
        sprintf_s(Buffer, "Height: %d", AssetMode->AddBitmap.Height);
        Label(&BitmapLayout, Buffer);

        v2 AlignPercentage = AssetMode->AssetToAdd.Source.AddBitmap.AlignPercentage;
        sprintf_s(Buffer, "AlignPercentage: %f, %f", AlignPercentage.x, AlignPercentage.y);
        Label(&BitmapLayout, Buffer);
        
        if(IsInRectangle(BitmapBounds, Layout->MouseP))
        {
            // NOTE(paul): Align Percentage
            v2 LocalMouseP = (Layout->MouseP - BitmapBounds.Min)*(1.0f / PixelDim);
            LocalMouseP = V2i(FloorReal32ToInt32(LocalMouseP.x), FloorReal32ToInt32(LocalMouseP.y));
            sprintf_s(Buffer, "BitmapMouseP: %f, %f", LocalMouseP.x, LocalMouseP.y);
            Label(&BitmapLayout, Buffer);

            if(AssetMode->PlaceAlignment)
            {
                AssetMode->AssetToAdd.Source.AddBitmap.AlignPercentage = V2(LocalMouseP.x / AssetMode->AddBitmap.Width,
                                                                            LocalMouseP.y / AssetMode->AddBitmap.Height);
                AssetMode->PlaceAlignment = false;
            }
        }    

        u32 AssetType = SimpleScrollElement(&BitmapLayout, &AssetMode->AssetTypeCursor,
                                            AdvanceArrayCursorInteraction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                                          &AssetMode->AssetTypeCursor, Asset_Count),
                                            V4(0.8f, 0.8f, 0.8f, 1), V4(1, 1, 1, 1), 4.0f, V4(0.0f, 0.635294117647f, 0.909803921569f, 1));

        u32 TagID = SimpleScrollElement(&BitmapLayout, &AssetMode->TagCursor,
                                        AdvanceArrayCursorInteraction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                                      &AssetMode->TagCursor, Tag_Count),
                                        V4(0.8f, 0.8f, 0.8f, 1), V4(1, 1, 1, 1), 4.0f, V4(0.0f, 0.635294117647f, 0.909803921569f, 1));
        EndLayout(&BitmapLayout);
        EndRow(Layout);
    }
    else
    {
        Label(Layout, "No Assets Found");
    }
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

    builder_assets *BuilderAssets = &AssetMode->BuilderAssets;
    
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
