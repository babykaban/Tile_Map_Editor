#if !defined(EDITOR_UI_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
struct ui_state;

struct interaction_id
{
    void *Value[2];
};

enum text_op
{
    TextOp_DrawText,
    TextOp_SizeText,
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

struct layout
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

#define EDITOR_UI_H
#endif
