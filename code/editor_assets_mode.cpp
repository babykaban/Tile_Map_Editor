/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

internal void
AssetAddModeBitmap(edit_mode_asset *AssetMode, ui_context *UIContext, layout *Layout)
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

    u32 Result = SimpleScrollElement(Layout, &AssetMode->TestCursor,
                                     AdvanceArrayCursorInteraction(UIItemIDFromEditMode(UIContext, EditMode_Assets),
                                                                   &AssetMode->TestCursor, AssetMode->BMPFileCount));
#if 0
    platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin(PlatformFileType_BMP, L"decorative/bmp/forest");
    wchar_t *FileName = Platform.GetNextFileName(&FileGroup);
    fclose(File);
    
    int a = 0;
    Platform.GetAllFilesOfTypeEnd(&FileGroup);
#endif
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
                          render_group *RenderGroup, game_input *Input, edit_mode_asset *AssetMode)
{
    v2 MouseP = Unproject(RenderGroup, DefaultFlatTransform(), V2i(Input->MouseX, Input->MouseY)).xy;
    layout Layout = BeginLayout(UIContext, MouseP, V2(UIContext->LeftEdge, 0.5f*UIContext->Height));
    
#if 0
#endif
    switch(AssetMode->AssetAddMode)
    {
        case AssetMode_Bitmap:
        {
            AssetAddModeBitmap(AssetMode, UIContext, &Layout);
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
