#if !defined(EDITOR_UI_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
//struct ui_state;
#if 0
#if 0
struct interaction_id
{
    u32 Value;
};
#endif

struct ui_context;

enum cursor_data_type
{
    CursorDataType_UInt32,
    CursorDataType_String,
    CursorDataType_StringArray,
};

struct array_cursor
{
    cursor_data_type DataType;
    u32 ArrayPosition;
    u32 ArrayCount;
    u32 Array[256];

    union
    {
        char **StringArray;
        char *String;
        u32 *U32Array;
    };
};

enum text_op
{
    TextOp_DrawText,
    TextOp_SizeText,
};

struct ui_item_id
{
    u32 ItemOwner;
    u32 ItemIndex;
};

struct view_inline_block
{
    v2 Dim;
};

enum view_type
{
    ViewType_Unknown,

    ViewType_Basic,
    ViewType_InlineBlock,
    ViewType_Collapsible,
};

struct ui_view
{
    ui_item_id ID;
    ui_view *NextInHash;

    view_type Type;
    union
    {
        view_inline_block InlineBlock;
    };
};

enum interaction_type
{
    Interaction_None,

    Interaction_NOP,

    Interaction_AutoModifyVariable,

    Interaction_ToggleValue,
    Interaction_DragValue,
    Interaction_TearValue,

    Interaction_Resize,
    Interaction_Move,

    Interaction_Select,

    Interaction_ToggleExpansion,

    Interaction_SetUInt32,
    Interaction_SetPointer,

    Interaction_AdvanceCursor,
};

struct ui_interaction
{
    ui_item_id ID;
    interaction_type Type;

    void *Target;
    union
    {
        void *Generic;
        void *Pointer;
        u32 UInt32;
        v2 *P;
    };
};

struct layout
{
    ui_context *UIContext;
    v2 MouseP;
    v2 BaseCorner;

    u32 Depth;

    v2 At;
    r32 LineAdvance;
    r32 NextYDelta;
    r32 SpacingX;
    r32 SpacingY;
    
    u32 NoLineFeed;
    b32 LineInitialized;
};

struct layout_element
{
    // NOTE(casey): Storage;
    layout *Layout;
    v2 *Dim;
    v2 *Size;
    ui_interaction Interaction;

    // NOTE(casey): Out
    rectangle2 Bounds;
};

struct ui_context
{
    memory_arena ContextArena;
    
    u32 DefaultClipRect;
    render_group RenderGroup;
    loaded_font *UIFont;
    ssa_font *UIFontInfo;

    object_transform TextTransform;
    object_transform ShadowTransform;
    object_transform UITransform;
    object_transform BackingTransform;

    v2 LastMouseP;
    b32 AltUI;

    r32 LeftEdge;
    r32 RightEdge;
    r32 FontScale;
    font_id FontID;
    r32 Width;
    r32 Height;

    ui_interaction Interaction;
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    layout MouseTextLayout;
    ui_view *ViewHash[1024];

    u32 UIItemCount;
};
#endif

struct ui_state;

enum cursor_data_type
{
    CursorDataType_UInt32,
    CursorDataType_String,
    CursorDataType_StringArray,
};

struct array_cursor
{
    cursor_data_type DataType;
    u32 ArrayPosition;
    u32 ArrayCount;
    u32 Array[256];

    union
    {
        char **StringArray;
        char *String;
        u32 *U32Array;
    };
};

struct interaction_id
{
    u32 Owner;
    u32 Value;
};

enum ui_text_op
{
    UITextOp_DrawText,
    UITextOp_SizeText,
};

enum interaction_type
{
    Interaction_None,

    Interaction_NOP,

    Interaction_AutoModifyVariable,

    Interaction_ToggleValue,
    Interaction_DragValue,
    Interaction_TearValue,

    Interaction_Resize,
    Interaction_Move,

    Interaction_Select,

    Interaction_ToggleExpansion,

    Interaction_SetUInt32,
    Interaction_SetPointer,
};

struct interaction
{
    interaction_id ID;
    interaction_type Type;

    void *Target;
    union
    {
        void *Generic;
        void *Pointer;
        u32 UInt32;
        v2 *P;
    };
};

struct ui_layout
{
    ui_state *UIState;
    v2 MouseP;
    v2 BaseCorner;

    u32 Depth;

    v2 At;
    r32 LineAdvance;
    r32 NextYDelta;
    r32 SpacingX;
    r32 SpacingY;
    
    u32 NoLineFeed;
    b32 LineInitialized;
};

struct ui_layout_element
{
    // NOTE(casey): Storage;
    ui_layout *Layout;
    v2 *Dim;
    v2 *Size;
    interaction Interaction;

    // NOTE(casey): Out
    rectangle2 Bounds;
};

struct ui_window
{
    v2 Dim;
    b32 Viewable;
};

struct ui_state
{
    b32 Initialized;
    memory_arena Arena;

    u32 DefaultClipRect;
    render_group RenderGroup;
    loaded_font *Font;
    ssa_font *FontInfo;

    object_transform TextTransform;
    object_transform ShadowTransform;
    object_transform UITransform;
    object_transform BackingTransform;

    v2 LastMouseP;
    b32 AltUI;
    s16 MouseZ;
    
    interaction Interaction;
    interaction HotInteraction;
    interaction NextHotInteraction;

    r32 LeftEdge;
    r32 RightEdge;
    r32 FontScale;
    font_id FontID;
    r32 GlobalWidth;
    r32 GlobalHeight;

    u32 NextInteractionID;
    ui_layout MouseTextLayout;
};

#define EDITOR_UI_H
#endif
