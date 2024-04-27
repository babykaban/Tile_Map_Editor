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
struct interaction_id
{
    u32 Value;
};
#endif

struct ui_context;

enum text_op
{
    TextOp_DrawText,
    TextOp_SizeText,
};

struct ui_item_id
{
    u32 Value;
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

enum ui_item_type
{
    UIItemType_None,
};

struct ui_item
{
    ui_item_id ID;
    ui_item_type ItemType;
};

struct ui_menu
{
    layout MenuLayout;
    ui_item MenuItems[32];
};

struct ui_context
{
    b32 Initialized;
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
    ui_menu ContextMenues[32];
    
    layout MouseTextLayout;
    ui_view *ViewHash[64];

    v2 WindowP;
};

#define EDITOR_UI_H
#endif
