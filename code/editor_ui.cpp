/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

inline b32
InteractionIDsAreEqual(interaction_id A, interaction_id B)
{
    b32 Result = ((A.Value[0] == B.Value[0]) &&
            (A.Value[1] == B.Value[1]));

    return(Result);
}


inline b32
InteractionsAreEqual(ui_interaction A, ui_interaction B)
{
    b32 Result = (InteractionIDsAreEqual(A.ID, B.ID) &&
            (A.Type == B.Type) &&
            (A.Target == B.Target) &&
            (A.Generic == B.Generic));

    return(Result);
}

inline b32
InteractionIsHot(ui_state *UIState, ui_interaction B)
{
    b32 Result = InteractionsAreEqual(UIState->HotInteraction, B);

    if(B.Type == Interaction_None)
    {
        Result = false;
    }

    return(Result);
}

inline b32
IsHex(char Char)
{
    b32 Result = (((Char >= '0') && (Char <= '9')) ||
                  ((Char >= 'A') && (Char <= 'F')));

    return(Result);
}

inline u32
GetHex(char Char)
{
    u32 Result = 0;

    if((Char >= '0') && (Char <= '9'))
    {
        Result = Char - '0';
    }
    else if((Char >= 'A') && (Char <= 'F'))
    {
        Result = 0xA + (Char - 'A');
    }

    return(Result);
}

internal rectangle2
TextOp(ui_state *UIState, text_op Op, v2 P, char *String, v4 Color = V4(1, 1, 1, 1),
    r32 AtZ = 0.0f)
{
    rectangle2 Result = InvertedInfinityRectangle2();
    if(UIState && UIState->UIFont)
    {
        render_group *RenderGroup = &UIState->RenderGroup;
        loaded_font *Font = UIState->UIFont;
        ssa_font *Info = UIState->UIFontInfo;

        u32 PrevCodePoint = 0;
        r32 CharScale = UIState->FontScale;
        r32 AtY = P.y;
        r32 AtX = P.x;
        for(char *At = String;
            *At;
            )
        {
            if((At[0] == '\\') &&
               (At[1] == '#') &&
               (At[2] != 0) &&
               (At[3] != 0) &&
               (At[4] != 0))
            {
                r32 CScale = 1.0f / 9.0f;
                Color = V4(Clamp01(CScale*(r32)(At[2] - '0')),
                           Clamp01(CScale*(r32)(At[3] - '0')),
                           Clamp01(CScale*(r32)(At[4] - '0')),
                           1.0f);
                At += 5;
            }
            else if((At[0] == '\\') &&
                    (At[1] == '^') &&
                    (At[2] != 0))
            {
                r32 CScale = 1.0f / 9.0f;
                CharScale = UIState->FontScale*Clamp01(CScale*(r32)(At[2] - '0'));
                At += 3;
            }
            else
            {
                u32 CodePoint = *At;
                if((At[0] == '\\') &&
                   (IsHex(At[1])) &&
                   (IsHex(At[2])) &&
                   (IsHex(At[3])) &&
                   (IsHex(At[4])))
                {
                    CodePoint = ((GetHex(At[1]) << 12) |
                                 (GetHex(At[2]) << 8) |
                                 (GetHex(At[3]) << 4) |
                                 (GetHex(At[4]) << 0));
                    At += 4;
                }

                r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
                AtX += AdvanceX;

                if(CodePoint != ' ')
                {
                    bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);
                    ssa_bitmap *BitmapInfo = GetBitmapInfo(RenderGroup->Assets, BitmapID);

                    r32 BitmapScale = CharScale*(r32)BitmapInfo->Dim[1];
                    v3 BitmapOffset = V3(AtX, AtY, AtZ);
                    if(Op == TextOp_DrawText)
                    {
                        PushBitmap(RenderGroup, UIState->TextTransform, BitmapID, BitmapScale,
                            BitmapOffset, Color, 1.0f);
                        PushBitmap(RenderGroup, UIState->ShadowTransform, BitmapID, BitmapScale,
                            BitmapOffset + V3(2.0f, -2.0f, 0.0f), V4(0, 0, 0, 1.0f), 1.0f);
                    }
                    else                    
                    {
                        Assert(Op == TextOp_SizeText);

                        loaded_bitmap *Bitmap = GetBitmap(RenderGroup->Assets, BitmapID, RenderGroup->GenerationID);
                        if(Bitmap)
                        {
                            used_bitmap_dim Dim = GetBitmapDim(RenderGroup, DefaultFlatTransform(), Bitmap, BitmapScale, BitmapOffset, 1.0f);
                            rectangle2 GlyphDim = RectMinDim(Dim.P.xy, Dim.Size);
                            Result = Union(Result, GlyphDim);
                        }
                    }
                }

                PrevCodePoint = CodePoint;

                ++At;
            }
        }
    }

    return(Result);
}

inline void
TextOutAt(ui_state *UIState, v2 P, char *String, v4 Color = V4(1, 1, 1, 1), r32 AtZ = 0.0f)
{
    render_group *RenderGroup = &UIState->RenderGroup;

    TextOp(UIState, TextOp_DrawText, P, String, Color, AtZ);    
}

inline rectangle2
GetTextSize(ui_state *UIState, char *String)
{
    rectangle2 Result = TextOp(UIState, TextOp_SizeText, V2(0, 0), String);

    return(Result);
}

inline r32
GetLineAdvance(ui_state *UIState)
{
    r32 Result = GetLineAdvanceFor(UIState->UIFontInfo)*UIState->FontScale;
    return(Result);
}

inline r32
GetBaseline(ui_state *UIState)
{
    r32 Result = UIState->FontScale*GetStartingBaselineY(UIState->UIFontInfo);
    return(Result);
}

inline ui_interaction
SetPointerInteraction(interaction_id DebugID, void **Target, void *Value)
{
    ui_interaction Result = {};
    Result.ID = DebugID;
    Result.Type = Interaction_SetPointer;
    Result.Target = Target;
    Result.Pointer = Value;

    return(Result);
}

inline ui_interaction
SetUInt32Interaction(interaction_id ID, u32 *Target, u32 Value)
{
    ui_interaction Result = {};
    Result.ID = ID;
    Result.Type = Interaction_SetUInt32;
    Result.Target = Target;
    Result.UInt32 = Value;

    return(Result);
}

inline layout 
BeginLayout(ui_state *UIState, v2 MouseP, v2 UpperLeftCorner)
{
    layout Layout = {};
    Layout.UIState = UIState;
    Layout.MouseP = MouseP;
    Layout.BaseCorner = Layout.At = UpperLeftCorner;
    Layout.LineAdvance = UIState->FontScale*GetLineAdvanceFor(UIState->UIFontInfo);
    Layout.SpacingY = 4.0f;
    Layout.SpacingX = 4.0f;
    
    return(Layout);
}

inline void
EndLayout(layout *Layout)
{
}

inline layout_element
BeginElementRectangle(layout *Layout, v2 *Dim)
{
    layout_element Element = {};

    Element.Layout = Layout;
    Element.Dim = Dim;

    return(Element);
}

inline void
MakeElementSizable(layout_element *Element)
{
    Element->Size = Element->Dim;
}

inline void
DefaultInteraction(layout_element *Element, ui_interaction Interaction)
{
    Element->Interaction = Interaction;
}

inline void
AdvanceElement(layout *Layout, rectangle2 ElRect)
{
    Layout->NextYDelta = Minimum(Layout->NextYDelta, GetMinCorner(ElRect).y - Layout->At.y);

    if(Layout->NoLineFeed)
    {
        Layout->At.x = GetMaxCorner(ElRect).x + Layout->SpacingX;
    }
    else
    {
        Layout->At.y += Layout->NextYDelta - Layout->SpacingY;
        Layout->LineInitialized = false;
    }
}

inline void
EndElement(layout_element *Element)
{
    layout *Layout = Element->Layout;
    ui_state *UIState = Layout->UIState;
    object_transform NoTransform = UIState->BackingTransform;
    
    if(!Layout->LineInitialized)
    {
        Layout->At.x = Layout->BaseCorner.x + Layout->Depth*2.0f*Layout->LineAdvance;
        Layout->LineInitialized = true;
        Layout->NextYDelta = 0.0f;
    }

    r32 SizeHandlePixels = 4.0f;

    v2 Frame = {0, 0};
    if(Element->Size)
    {
        Frame.x = SizeHandlePixels;
        Frame.y = SizeHandlePixels;
    }

    v2 TotalDim = *Element->Dim + 2.0f*Frame;

    v2 TotalMinCorner = V2(Layout->At.x,
                           Layout->At.y - TotalDim.y);
    v2 TotalMaxCorner = TotalMinCorner + TotalDim;

    v2 InteriorMinCorner = TotalMinCorner + Frame;
    v2 InteriorMaxCorner = InteriorMinCorner + *Element->Dim;

    rectangle2 TotalBounds = RectMinMax(TotalMinCorner, TotalMaxCorner);
    Element->Bounds = RectMinMax(InteriorMinCorner, InteriorMaxCorner);

    if(Element->Interaction.Type && IsInRectangle(Element->Bounds, Layout->MouseP))
    {
        UIState->NextHotInteraction = Element->Interaction;
    }

    if(Element->Size)
    {
        PushRect(&UIState->RenderGroup, NoTransform, RectMinMax(V2(TotalMinCorner.x, InteriorMinCorner.y),
                V2(InteriorMinCorner.x, InteriorMaxCorner.y)), 0.0f,
                 V4(0, 0, 0, 1));
        PushRect(&UIState->RenderGroup, NoTransform, RectMinMax(V2(InteriorMaxCorner.x, InteriorMinCorner.y),
                                                     V2(TotalMaxCorner.x, InteriorMaxCorner.y)), 0.0f,
                 V4(0, 0, 0, 1));
        PushRect(&UIState->RenderGroup, NoTransform, RectMinMax(V2(InteriorMinCorner.x, TotalMinCorner.y),
                                                     V2(InteriorMaxCorner.x, InteriorMinCorner.y)), 0.0f,
                 V4(0, 0, 0, 1));
        PushRect(&UIState->RenderGroup, NoTransform, RectMinMax(V2(InteriorMinCorner.x, InteriorMaxCorner.y),
                                                     V2(InteriorMaxCorner.x, TotalMaxCorner.y)), 0.0f,
                 V4(0, 0, 0, 1));

        ui_interaction SizeInteraction = {};
        SizeInteraction.Type = Interaction_Resize;
        SizeInteraction.P = Element->Size;

        rectangle2 SizeBox = AddRadiusTo(
            RectMinMax(V2(InteriorMaxCorner.x, TotalMinCorner.y),
                V2(TotalMaxCorner.x, InteriorMinCorner.y)), V2(4.0f, 4.0f));
        PushRect(&UIState->RenderGroup, NoTransform, SizeBox, 0.0f,
                 (InteractionIsHot(UIState, SizeInteraction) ? V4(1, 1, 0, 1) : V4(1, 1, 1, 1)));
        if(IsInRectangle(SizeBox, Layout->MouseP))
        {
            UIState->NextHotInteraction = SizeInteraction;
        }
    }

    AdvanceElement(Layout, TotalBounds);
}

internal v2
BasicTextElement(layout *Layout, char *Text, ui_interaction ItemInteraction,
                 v4 ItemColor = V4(0.8f, 0.8f, 0.8f, 1), v4 HotColor = V4(1, 1, 1, 1),
                 r32 Border = 0.0f, v4 BackdropColor = V4(0, 0, 0, 0))
{
    ui_state *UIState = Layout->UIState;

    rectangle2 TextBounds = GetTextSize(UIState, Text);
    v2 Dim = {GetDim(TextBounds).x + 2.0f*Border, Layout->LineAdvance + 2.0f*Border};
    
    layout_element Element = BeginElementRectangle(Layout, &Dim);
    DefaultInteraction(&Element, ItemInteraction);
    EndElement(&Element);

    b32 IsHot = InteractionIsHot(Layout->UIState, ItemInteraction);

    TextOutAt(UIState, V2(GetMinCorner(Element.Bounds).x + Border,
                GetMaxCorner(Element.Bounds).y - Border - 
                UIState->FontScale*GetStartingBaselineY(UIState->UIFontInfo)),
            Text, IsHot ? HotColor : ItemColor);
    if(BackdropColor.w > 0.0f)
    {
        PushRect(&UIState->RenderGroup, 
                 UIState->BackingTransform, Element.Bounds, 0.0f, BackdropColor);
    }
    
    return(Dim);
}

internal void
BeginRow(layout *Layout)
{
    ++Layout->NoLineFeed;
}

internal void
Label(layout *Layout, char *Name)
{
    ui_interaction NullInteraction = {};
    BasicTextElement(Layout, Name, NullInteraction, V4(1, 1, 1, 1), V4(1, 1, 1, 1));
}

internal void
ActionButton(layout *Layout, char *Name, ui_interaction Interaction)
{
    BasicTextElement(Layout, Name, Interaction, 
        V4(0.5f, 0.5f, 0.5f, 1.0f), V4(1, 1, 1, 1),
        4.0f, V4(0, 0.5f, 1.0f, 1.0f));
}

internal void
BooleanButton(layout *Layout, char *Name, b32 Highlight, ui_interaction Interaction)
{
    BasicTextElement(Layout, Name, Interaction, 
        Highlight ? V4(1, 1, 1, 1) : V4(0.5f, 0.5f, 0.5f, 1.0f), V4(1, 1, 1, 1),
        4.0f, V4(0.0f, 0.5f, 1.0f, 1.0f));
}

internal void
EndRow(layout *Layout)
{
    Assert(Layout->NoLineFeed > 0);
    --Layout->NoLineFeed;

    AdvanceElement(Layout, RectMinMax(Layout->At, Layout->At));
}

internal void
AddTooltip(ui_state *UIState, char *Text)
{
    render_group *RenderGroup = &UIState->RenderGroup;
    u32 OldClipRect = RenderGroup->CurrentClipRectIndex;
    RenderGroup->CurrentClipRectIndex = UIState->DefaultClipRect;
    
    layout *Layout = &UIState->MouseTextLayout;
    
    rectangle2 TextBounds = GetTextSize(UIState, Text);
    v2 Dim = {GetDim(TextBounds).x, Layout->LineAdvance};
    
    layout_element Element = BeginElementRectangle(Layout, &Dim);
    EndElement(&Element);
    
    TextOutAt(UIState, V2(GetMinCorner(Element.Bounds).x,
                GetMaxCorner(Element.Bounds).y - 
                UIState->FontScale*GetStartingBaselineY(UIState->UIFontInfo)),
            Text, V4(1, 1, 1, 1), 10000.0f);
        
    RenderGroup->CurrentClipRectIndex = OldClipRect;
}

internal void
BeginInteract(ui_state *UIState, game_input *Input, v2 MouseP)
{
    if(UIState->HotInteraction.Type)
    {
        if(UIState->HotInteraction.Type == Interaction_AutoModifyVariable)
        {
            switch(UIState->HotInteraction.Element->Frames[FrameOrdinal].MostRecentEvent->Event.Type)
            {
                case DebugType_b32:
                {
                    UIState->HotInteraction.Type = DebugInteraction_ToggleValue;
                } break;

                case DebugType_r32:
                {
                    UIState->HotInteraction.Type = DebugInteraction_DragValue;
                } break;

                case DebugType_OpenDataBlock:
                {
                    UIState->HotInteraction.Type = DebugInteraction_ToggleValue;
                } break;
            }
        }

        switch(UIState->HotInteraction.Type)
        {
            case DebugInteraction_TearValue:
            {
                debug_variable_link *RootGroup = CloneVariableLink(UIState, UIState->HotInteraction.Link);
                debug_tree *Tree = AddTree(UIState, RootGroup, MouseP);
                UIState->HotInteraction.Type = DebugInteraction_Move;
                UIState->HotInteraction.P = &Tree->UIP;
            } break;

            case DebugInteraction_Select:
            {
                if(!Input->ShiftDown)
                {
                    ClearSelection(UIState);
                }
                AddToSelection(UIState, UIState->HotInteraction.ID);
            } break;                
        }

        UIState->Interaction = UIState->HotInteraction;
    }
    else
    {
        UIState->Interaction.Type = DebugInteraction_NOP;
    }
}

internal void
Interact(ui_state *UIState, game_input *Input, v2 MouseP)
{
    v2 dMouseP = MouseP - UIState->LastMouseP;
    if(UIState->Interaction.Type)
    {
        v2 *P = UIState->Interaction.P;

        // NOTE(casey): Mouse move interaction
        switch(UIState->Interaction.Type)
        {
            case Interaction_DragValue:
            {
            } break;

            case Interaction_Resize:
            {
                *P += V2(dMouseP.x, -dMouseP.y);
                P->x = Maximum(P->x, 10.0f);
                P->y = Maximum(P->y, 10.0f);
            } break;

            case Interaction_Move:
            {
                *P += V2(dMouseP.x, dMouseP.y);
            } break;
        }

        // NOTE(casey): Click interaction
        for(u32 TransitionIndex = Input->MouseButtons[0].HalfTransitionCount;
            TransitionIndex > 1;
            --TransitionIndex)
        {
            EndInteract(UIState, Input, MouseP);
            BeginInteract(UIState, Input, MouseP);
        }

        if(!Input->MouseButtons[0].EndedDown)
        {
            EndInteract(UIState, Input, MouseP);
        }
    }
    else
    {
        UIState->HotInteraction = UIState->NextHotInteraction;

        for(u32 TransitionIndex = Input->MouseButtons[0].HalfTransitionCount;
            TransitionIndex > 1;
            --TransitionIndex)
        {
            BeginInteract(UIState, Input, MouseP);
            EndInteract(UIState, Input, MouseP);
        }

        if(Input->MouseButtons[0].EndedDown)
        {
            BeginInteract(UIState, Input, MouseP);
        }
    }

    UIState->LastMouseP = MouseP;
}

internal void
BeginUI(ui_state *UIState, game_render_commands *Commands, game_assets *Assets, u32 MainGenerationID, u32 Width, u32 Height)
{
    if(!UIState->Initialized)
    {
//        memory_index TotalMemorySize = DebugGlobalMemory->DebugStorageSize - sizeof(debug_state);
//        InitializeArena(&UIState->UIArena, TotalMemorySize, UIState + 1);
//        SubArena(&UIState->PerFrameArena, &UIState->DebugArena, (TotalMemorySize / 2));
        UIState->Paused = false;
        
        UIState->Initialized = true;
    }

    UIState->RenderGroup = BeginRenderGroup(Assets, Commands, MainGenerationID, false);

    UIState->UIFont = PushFont(&UIState->RenderGroup, UIState->FontID);
    UIState->UIFontInfo = GetFontInfo(UIState->RenderGroup.Assets, UIState->FontID);

    UIState->GlobalWidth = (r32)Width;
    UIState->GlobalHeight = (r32)Height;

    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
    WeightVector.E[Tag_FontType] = 1.0f;
    UIState->FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);

    UIState->FontScale = 1.0f;
    Orthographic(&UIState->RenderGroup, Width, Height, 1.0f);
    UIState->LeftEdge = -0.5f*Width;
    UIState->RightEdge = 0.5f*Width;

    UIState->TextTransform = DefaultFlatTransform();
    UIState->ShadowTransform = DefaultFlatTransform();
    UIState->UITransform = DefaultFlatTransform();
    UIState->BackingTransform = DefaultFlatTransform();

    UIState->BackingTransform.SortBias = 100000.0f;
    UIState->ShadowTransform.SortBias = 200000.0f;
    UIState->UITransform.SortBias = 300000.0f;
    UIState->TextTransform.SortBias = 400000.0f;

    UIState->DefaultClipRect = UIState->RenderGroup.CurrentClipRectIndex;
}

internal void
EndUI(ui_state *UIState, game_input *Input)
{
    render_group *RenderGroup = &UIState->RenderGroup;

    UIState->AltUI = Input->MouseButtons[1].EndedDown;
    v2 MouseP = Unproject(RenderGroup, DefaultFlatTransform(), V2i(Input->MouseX, Input->MouseY)).xy;
    UIState->MouseTextLayout = BeginLayout(UIState, MouseP, MouseP);
    EndLayout(&UIState->MouseTextLayout);
    Interact(UIState, Input, MouseP);
    
    EndRenderGroup(&UIState->RenderGroup);

    // NOTE(casey): Clear the UI state for the next frame
    ZeroStruct(UIState->NextHotInteraction);
}
