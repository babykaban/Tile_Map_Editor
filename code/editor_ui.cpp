/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

inline ui_interaction
SetPointerInteraction(ui_item_id ID, void **Target, void *Value)
{
    ui_interaction Result = {};
    Result.ID = ID;
    Result.Type = Interaction_SetPointer;
    Result.Target = Target;
    Result.Pointer = Value;

    return(Result);
}

inline ui_interaction
SetUInt32Interaction(ui_item_id ID, u32 *Target, u32 Value)
{
    ui_interaction Result = {};
    Result.ID = ID;
    Result.Type = Interaction_SetUInt32;
    Result.Target = Target;
    Result.UInt32 = Value;

    return(Result);
}

inline ui_interaction
AdvanceArrayCursorInteraction(ui_item_id ID, array_cursor *Target, u32 SourceArrayCount)
{
    ui_interaction Result = {};
    Result.ID = ID;
    Result.Type = Interaction_AdvanceCursor;
    Result.Target = Target;
    Result.UInt32 = SourceArrayCount;

    return(Result);
}

inline b32
InteractionIDsAreEqual(ui_item_id A, ui_item_id B)
{
    b32 Result = ((A.Value == B.Value) &&
                  (A.ItemOwner == B.ItemOwner) &&
                  (A.ItemIndex == B.ItemIndex));

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
InteractionIsHot(ui_context *UIContext, ui_interaction B)
{
    b32 Result = InteractionsAreEqual(UIContext->HotInteraction, B);

    if(B.Type == Interaction_None)
    {
        Result = false;
    }

    return(Result);
}

internal void
ResetCursorArray(array_cursor *Cursor)
{
    for(u32 I = 0;
        I < Cursor->ArrayCount;
        ++I)
    {
        Cursor->Array[I] = I;
    }
}

internal void
InitializeCursor(array_cursor *Cursor, u32 CursorArrayCount)
{
    Cursor->ArrayPosition = 0;
    Cursor->ArrayCount = CursorArrayCount;
    ResetCursorArray(Cursor);
}

inline  void
InitializeStringArrayCursor(array_cursor *Cursor, u32 CursorArrayCount, char **Data)
{
    Cursor->DataType = CursorDataType_StringArray;
    Cursor->StringArray = Data;
    InitializeCursor(Cursor, CursorArrayCount);
}

internal void
ReInitializeCursor(array_cursor *Cursor, u32 CursorArrayCount)
{
    Cursor->ArrayPosition = 0;
    Cursor->ArrayCount = CursorArrayCount;
    ResetCursorArray(Cursor);
}

internal void
ChangeCursorPositionFor(array_cursor *Cursor, u32 SourceArrayCount, s16 MouseZ)
{
    if(MouseZ != 0)
    {
        u32 CursorLastIndex = Cursor->ArrayCount - 1;
        s32 Count = MouseZ > 0 ? MouseZ : MouseZ * -1;
        for(s32 MouseRotIndex = 0;
            MouseRotIndex < Count;
            ++MouseRotIndex)
        {
            if(MouseZ > 0)
            {
                if(Cursor->ArrayPosition == 0)
                {
                    s32 NewFirst = Cursor->Array[0] - 1;
                    for(u32 I = CursorLastIndex;
                        I > 0;
                        --I)
                    {
                        Cursor->Array[I] = Cursor->Array[I - 1];
                    }

                    if(NewFirst == -1)
                    {
                        NewFirst = SourceArrayCount - 1;
                    }
                    Cursor->Array[0] = NewFirst;
                }
                else
                {
                    --Cursor->ArrayPosition;
                }
            }
            else
            {
                if(Cursor->ArrayPosition == CursorLastIndex)
                {
                    u32 NewLast = Cursor->Array[CursorLastIndex] + 1;
                    for(u32 I = 0;
                        I < Cursor->ArrayCount;
                        ++I)
                    {
                        Cursor->Array[I] = Cursor->Array[I + 1];
                    }

                    if(NewLast == SourceArrayCount)
                    {
                        NewLast = 0;
                    }
                    Cursor->Array[CursorLastIndex] = NewLast;
                }
                else
                {
                    ++Cursor->ArrayPosition;
                }
            }
        }
    }
}

internal ui_view *
GetOrCreateDebugViewFor(ui_context *UIContext, ui_item_id ID)
{
    // TODO(casey): Better hash function
    u32 HashIndex = ((ID.Value >> 2) + (ID.ItemOwner >> 2) + (ID.ItemIndex >> 2)) % ArrayCount(UIContext->ViewHash);
    ui_view **HashSlot = UIContext->ViewHash + HashIndex;

    ui_view *Result = 0;
    for(ui_view *Search = *HashSlot;
        Search;
        Search = Search->NextInHash)
    {
        if(InteractionIDsAreEqual(Search->ID, ID))
        {
            Result = Search;
            break;
        }
    }

    if(!Result)
    {
        Result = PushStruct(&UIContext->ContextArena, ui_view);
        Result->ID = ID;
        Result->Type = ViewType_Unknown;
        Result->NextInHash = *HashSlot;
        *HashSlot = Result;
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
TextOp(ui_context *UIContext, text_op Op, v2 P, char *String, v4 Color = V4(1, 1, 1, 1),
    r32 AtZ = 0.0f)
{
    rectangle2 Result = InvertedInfinityRectangle2();
    if(UIContext && UIContext->UIFont)
    {
        render_group *RenderGroup = &UIContext->RenderGroup;
        loaded_font *Font = UIContext->UIFont;
        ssa_font *Info = UIContext->UIFontInfo;

        u32 PrevCodePoint = 0;
        r32 CharScale = UIContext->FontScale;
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
                CharScale = UIContext->FontScale*Clamp01(CScale*(r32)(At[2] - '0'));
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
                        PushBitmap(RenderGroup, UIContext->TextTransform, BitmapID, BitmapScale,
                            BitmapOffset, Color, 1.0f);
                        PushBitmap(RenderGroup, UIContext->ShadowTransform, BitmapID, BitmapScale,
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
TextOutAt(ui_context *UIContext, v2 P, char *String, v4 Color = V4(1, 1, 1, 1), r32 AtZ = 0.0f)
{
    render_group *RenderGroup = &UIContext->RenderGroup;

    TextOp(UIContext, TextOp_DrawText, P, String, Color, AtZ);    
}

inline rectangle2
GetTextSize(ui_context *UIContext, char *String)
{
    rectangle2 Result = TextOp(UIContext, TextOp_SizeText, V2(0, 0), String);

    return(Result);
}

inline r32
GetLineAdvance(ui_context *UIContext)
{
    r32 Result = GetLineAdvanceFor(UIContext->UIFontInfo)*UIContext->FontScale;
    return(Result);
}

inline r32
GetBaseline(ui_context *UIContext)
{
    r32 Result = UIContext->FontScale*GetStartingBaselineY(UIContext->UIFontInfo);
    return(Result);
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
    ui_context *UIContext = Layout->UIContext;
    object_transform NoTransform = UIContext->BackingTransform;
    
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
        UIContext->NextHotInteraction = Element->Interaction;
    }

    if(Element->Size)
    {
        PushRect(&UIContext->RenderGroup, NoTransform, RectMinMax(V2(TotalMinCorner.x, InteriorMinCorner.y),
                V2(InteriorMinCorner.x, InteriorMaxCorner.y)), 0.0f,
                 V4(0, 0, 0, 1));
        PushRect(&UIContext->RenderGroup, NoTransform, RectMinMax(V2(InteriorMaxCorner.x, InteriorMinCorner.y),
                                                     V2(TotalMaxCorner.x, InteriorMaxCorner.y)), 0.0f,
                 V4(0, 0, 0, 1));
        PushRect(&UIContext->RenderGroup, NoTransform, RectMinMax(V2(InteriorMinCorner.x, TotalMinCorner.y),
                                                     V2(InteriorMaxCorner.x, InteriorMinCorner.y)), 0.0f,
                 V4(0, 0, 0, 1));
        PushRect(&UIContext->RenderGroup, NoTransform, RectMinMax(V2(InteriorMinCorner.x, InteriorMaxCorner.y),
                                                     V2(InteriorMaxCorner.x, TotalMaxCorner.y)), 0.0f,
                 V4(0, 0, 0, 1));

        ui_interaction SizeInteraction = {};
        SizeInteraction.Type = Interaction_Resize;
        SizeInteraction.P = Element->Size;

        rectangle2 SizeBox = AddRadiusTo(
            RectMinMax(V2(InteriorMaxCorner.x, TotalMinCorner.y),
                V2(TotalMaxCorner.x, InteriorMinCorner.y)), V2(4.0f, 4.0f));
        PushRect(&UIContext->RenderGroup, NoTransform, SizeBox, 0.0f,
                 (InteractionIsHot(UIContext, SizeInteraction) ? V4(1, 1, 0, 1) : V4(1, 1, 1, 1)));
        if(IsInRectangle(SizeBox, Layout->MouseP))
        {
            UIContext->NextHotInteraction = SizeInteraction;
        }
    }

    AdvanceElement(Layout, TotalBounds);
}

inline layout 
BeginLayout(ui_context *UIContext, v2 MouseP, v2 UpperLeftCorner)
{
    layout Layout = {};
    Layout.UIContext = UIContext;
    Layout.MouseP = MouseP;
    Layout.BaseCorner = Layout.At = UpperLeftCorner;
    Layout.LineAdvance = UIContext->FontScale*GetLineAdvanceFor(UIContext->UIFontInfo);
    Layout.SpacingY = 4.0f;
    Layout.SpacingX = 4.0f;
    
    return(Layout);
}

inline void
EndLayout(layout *Layout)
{
}

internal v2
BasicTextElement(layout *Layout, char *Text, ui_interaction ItemInteraction,
                 v4 ItemColor = V4(0.8f, 0.8f, 0.8f, 1), v4 HotColor = V4(1, 1, 1, 1),
                 r32 Border = 0.0f, v4 BackdropColor = V4(0, 0, 0, 0))
{
    ui_context *UIContext = Layout->UIContext;

    rectangle2 TextBounds = GetTextSize(UIContext, Text);
    v2 Dim = {GetDim(TextBounds).x + 2.0f*Border, Layout->LineAdvance + 2.0f*Border};
    
    layout_element Element = BeginElementRectangle(Layout, &Dim);
    DefaultInteraction(&Element, ItemInteraction);
    EndElement(&Element);

    b32 IsHot = InteractionIsHot(Layout->UIContext, ItemInteraction);

    TextOutAt(UIContext, V2(GetMinCorner(Element.Bounds).x + Border,
                GetMaxCorner(Element.Bounds).y - Border - 
                UIContext->FontScale*GetStartingBaselineY(UIContext->UIFontInfo)),
            Text, IsHot ? HotColor : ItemColor);
    if(BackdropColor.w > 0.0f)
    {
        PushRect(&UIContext->RenderGroup, 
                 UIContext->BackingTransform, Element.Bounds, 0.0f, BackdropColor);
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
BeginInteract(ui_context *UIContext, game_input *Input, v2 MouseP)
{
    if(UIContext->HotInteraction.Type)
    {
        switch(UIContext->HotInteraction.Type)
        {
            case Interaction_TearValue:
            {
            } break;

            case Interaction_Select:
            {
            } break;                

            case Interaction_AdvanceCursor:
            {
                array_cursor *Cursor = (array_cursor *)UIContext->Interaction.Target;
                if(Cursor)
                {
                    ChangeCursorPositionFor(Cursor, UIContext->HotInteraction.UInt32, Input->MouseZ);
                }
            } break;
        }

        UIContext->Interaction = UIContext->HotInteraction;
    }
    else
    {
        UIContext->Interaction.Type = Interaction_NOP;
    }
}

internal void
EndInteract(ui_context *UIContext, game_input *Input, v2 MouseP)
{
    switch(UIContext->Interaction.Type)
    {
        case Interaction_ToggleExpansion:
        {
        } break;
        
        case Interaction_SetUInt32:
        {
            *(u32 *)UIContext->Interaction.Target = UIContext->Interaction.UInt32;
        } break;
        
        case Interaction_SetPointer:
        {
        } break;

        case Interaction_ToggleValue:
        {
            *(u32 *)UIContext->Interaction.Target = UIContext->Interaction.UInt32;
        } break;
    }

    UIContext->Interaction.Type = Interaction_None;
    UIContext->Interaction.Generic = 0;
}

internal void
Interact(ui_context *UIContext, game_input *Input, v2 MouseP)
{
    v2 dMouseP = MouseP - UIContext->LastMouseP;
    if(UIContext->Interaction.Type)
    {
        v2 *P = UIContext->Interaction.P;

        // NOTE(casey): Mouse move interaction
        switch(UIContext->Interaction.Type)
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
            EndInteract(UIContext, Input, MouseP);
            BeginInteract(UIContext, Input, MouseP);
        }

        if(!Input->MouseButtons[0].EndedDown)
        {
            EndInteract(UIContext, Input, MouseP);
        }
    }
    else
    {
        UIContext->HotInteraction = UIContext->NextHotInteraction;
#if 0
        for(u32 TransitionIndex = Input->MouseButtons[0].HalfTransitionCount;
            TransitionIndex > 1;
            --TransitionIndex)
        {
            BeginInteract(UIContext, Input, MouseP);
            EndInteract(UIContext, Input, MouseP);
        }
#endif

        if(Input->MouseButtons[0].EndedDown)
        {
            BeginInteract(UIContext, Input, MouseP);
        }

        if((Input->MouseZ != 0) && (UIContext->HotInteraction.Type == Interaction_AdvanceCursor))
        {
            BeginInteract(UIContext, Input, MouseP);
        }
        
    }

    UIContext->LastMouseP = MouseP;
}

internal void
BeginUI(ui_context *UIContext, game_render_commands *Commands, game_assets *Assets, u32 MainGenerationID,
        u32 ContextWidth, u32 ContextHeight)
{
    if(!UIContext->Initialized)
    {
        UIContext->Initialized = true;
    }

    UIContext->RenderGroup = BeginRenderGroup(Assets, Commands, MainGenerationID, false);

    UIContext->UIFont = PushFont(&UIContext->RenderGroup, UIContext->FontID);
    UIContext->UIFontInfo = GetFontInfo(UIContext->RenderGroup.Assets, UIContext->FontID);

    UIContext->Width = (r32)ContextWidth;
    UIContext->Height = (r32)ContextHeight;

    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
    WeightVector.E[Tag_FontType] = 1.0f;
    UIContext->FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);

    UIContext->FontScale = 1.0f;
    Orthographic(&UIContext->RenderGroup, ContextWidth, ContextHeight, 1.0f);
    UIContext->LeftEdge = -0.5f*ContextWidth;
    UIContext->RightEdge = 0.5f*ContextWidth;

    UIContext->TextTransform = DefaultFlatTransform();
    UIContext->ShadowTransform = DefaultFlatTransform();
    UIContext->UITransform = DefaultFlatTransform();
    UIContext->BackingTransform = DefaultFlatTransform();

    UIContext->BackingTransform.SortBias = 100000.0f;
    UIContext->ShadowTransform.SortBias = 200000.0f;
    UIContext->UITransform.SortBias = 300000.0f;
    UIContext->TextTransform.SortBias = 400000.0f;

    UIContext->DefaultClipRect = UIContext->RenderGroup.CurrentClipRectIndex;
}

internal void 
DrawWindow(ui_context *UIContext, layout *Layout, v2 *D, ui_interaction WindowInteraction)
{
    render_group *RenderGroup = &UIContext->RenderGroup;

    layout_element WindowElement = BeginElementRectangle(Layout, D);
    DefaultInteraction(&WindowElement, WindowInteraction);
    MakeElementSizable(&WindowElement);
    EndElement(&WindowElement);

    v3 InitOffsetP = UIContext->BackingTransform.OffsetP;
    v2 Center = GetCenter(WindowElement.Bounds);
    v2 Dim = GetDim(WindowElement.Bounds);
    
    UIContext->BackingTransform.OffsetP += V3(Center, 0.0f);
    v2 HalfDim = 0.5f*Dim;
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 0), Dim,
             V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 2.0f), Dim - V2(4.0f, 4.0f),
             V4(0.725490196078f, 0.478431372549f, 0.341176470588f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 3.0f), Dim - V2(12.0f, 12.0f),
             V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 4.0f), Dim - V2(16.0f, 16.0f),
             V4(0.403921568627f, 0.254901960784f, 0.172549019608f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 5.0f), Dim - V2(24.0f, 24.0f),
             V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 6.0f), Dim - V2(28.0f, 28.0f),
             V4(0.725490196078f, 0.478431372549f, 0.341176470588f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 7.0f), Dim - V2(32.0f, 32.0f),
             V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 8.0f), Dim - V2(36.0f, 36.0f),
             V4(0.403921568627f, 0.254901960784f, 0.172549019608f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, 0, 9.0f), Dim - V2(44.0f, 44.0f),
             V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));

    r32 YOffset = HalfDim.y - 42.0f;
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, YOffset, 10.0f), V2(Dim.x - 44.0f, 48.0f),
             V4(0.403921568627f, 0.254901960784f, 0.172549019608f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, YOffset, 11.0f), V2(Dim.x - 44.0f, 40.0f),
             V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, YOffset, 12.0f), V2(Dim.x - 48.0f, 36.0f),
             V4(0.725490196078f, 0.478431372549f, 0.341176470588f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, YOffset, 13.0f), V2(Dim.x - 52.0f, 32.0f),
             V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0, YOffset, 14.0f), V2(Dim.x - 56.0f, 28.0f),
             V4(0.811764705882f, 0.933333333333f, 0.960784313725f, 1));

    PushRect(RenderGroup, UIContext->BackingTransform, V3(0.5f, HalfDim.y - 43.0f, 15.0f), V2(Dim.x - 58.0f, 26.0f),
             V4(0.0f, 0.635294117647f, 0.909803921569f, 1));
    PushRect(RenderGroup, UIContext->BackingTransform, V3(0.0f, YOffset, 16.0f), V2(Dim.x - 60.0f, 24.0f),
             V4(0.6f, 0.850980392157f, 0.917647058824f, 1));

    r32 Ax[2] = {HalfDim.x - 12.0f, -(HalfDim.x - 12.0f)};
    r32 Ay[2] = {HalfDim.y - 3.0f, -(HalfDim.y - 3.0f)};

    r32 Bx[2] = {HalfDim.x - 3.0f, -(HalfDim.x - 3.0f)};
    r32 By[2] = {HalfDim.y - 12.0f, -(HalfDim.y - 12.0f)};

    r32 Cx[2] = {HalfDim.x - 10.0f, -(HalfDim.x - 10.0f)};
    r32 Cy[2] = {HalfDim.y - 10.0f, -(HalfDim.y - 10.0f)};

    r32 Dx[2] = {HalfDim.x - 8.0f, -(HalfDim.x - 8.0f)};
    r32 Dy[2] = {HalfDim.y - 8.0f, -(HalfDim.y - 8.0f)};

    r32 Ex[2] = {HalfDim.x - 1.0f, -(HalfDim.x - 1.0f)};
    r32 Ey[2] = {HalfDim.y - 1.0f, -(HalfDim.y - 1.0f)};

    r32 Fx[2] = {HalfDim.x - 5.0f, -(HalfDim.x - 5.0f)};
    r32 Fy[2] = {HalfDim.y - 6.0f, -(HalfDim.y - 6.0f)};

    r32 Gx[2] = {HalfDim.x - 11.0f, -(HalfDim.x - 11.0f)};
    r32 Gy[2] = {HalfDim.y - 7.0f, -(HalfDim.y - 7.0f)};

    r32 Hx[2] = {HalfDim.x - 13.0f, -(HalfDim.x - 13.0f)};

    u32 Indecies[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};

    for(u32 Itter = 0;
        Itter < 4;
        ++Itter)
    {
        u32 IndexX = Indecies[Itter][0];
        u32 IndexY = Indecies[Itter][1];

        PushRect(RenderGroup, UIContext->BackingTransform, V3(Ax[IndexX], Ay[IndexY], 10.0f), V2(32.0f, 14.0f),
                 V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Bx[IndexX], By[IndexY], 10.0f), V2(14.0f, 32.0f),
                 V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));

        PushRect(RenderGroup, UIContext->BackingTransform, V3(Ax[IndexX], Ay[IndexY], 11.0f), V2(28.0f, 10.0f),
                 V4(0.725490196078f, 0.478431372549f, 0.341176470588f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Bx[IndexX], By[IndexY], 11.0f), V2(10.0f, 28.0f),
                 V4(0.725490196078f, 0.478431372549f, 0.341176470588f, 1));

        PushRect(RenderGroup, UIContext->BackingTransform, V3(Cx[IndexX], Cy[IndexY], 12.0f), V2(12.0f, 12.0f),
                 V4(0.247058823529f, 0.282352941176f, 0.8f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Dx[IndexX], Dy[IndexY], 12.0f), V2(12.0f, 12.0f),
                 V4(0.247058823529f, 0.282352941176f, 0.8f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Dx[IndexX], Ey[IndexY], 12.0f), V2(8.0f, 2.0f),
                 V4(0.247058823529f, 0.282352941176f, 0.8f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Ex[IndexX], Dy[IndexY], 12.0f), V2(2.0f, 8.0f),
                 V4(0.247058823529f, 0.282352941176f, 0.8f, 1));

        PushRect(RenderGroup, UIContext->BackingTransform, V3(Bx[IndexX], Dy[IndexY], 13.0f), V2(2.0f, 8.0f),
                 V4(0.6f, 0.850980392157f, 0.917647058824f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Fx[IndexX], Dy[IndexY], 13.0f), V2(2.0f, 12.0f),
                 V4(0.6f, 0.850980392157f, 0.917647058824f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Dx[IndexX], Fy[IndexY], 13.0f), V2(4.0f, 8.0f),
                 V4(0.6f, 0.850980392157f, 0.917647058824f, 1));

        PushRect(RenderGroup, UIContext->BackingTransform, V3(Gx[IndexX], Fy[IndexY], 14.0f), V2(2.0f, 8.0f),
                 V4(0.0f, 0.635294117647f, 0.909803921569f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Hx[IndexX], Gy[IndexY], 13.0f), V2(2.0f, 6.0f),
                 V4(0.0f, 0.635294117647f, 0.909803921569f, 1));
        PushRect(RenderGroup, UIContext->BackingTransform, V3(Cx[IndexX], By[IndexY], 13.0f), V2(8.0f, 4.0f),
                 V4(0.0f, 0.635294117647f, 0.909803921569f, 1));

        PushRect(RenderGroup, UIContext->BackingTransform, V3(Dx[IndexX], Dy[IndexY], 14.0f), V2(4.0f, 4.0f),
                 V4(1, 1, 1, 1));
    }

    UIContext->BackingTransform.OffsetP = InitOffsetP;
}

internal u32
SimpleScrollElement(layout *Layout, array_cursor *Cursor, v2 ElementDim, ui_interaction ItemInteraction,
                r32 Border = 0.0f)
{
    u32 SourceArrayIndex = Cursor->Array[Cursor->ArrayPosition];
    u32 ElementsToShow = Cursor->ArrayCount;
    for(u32 Index = 0;
        Index < ElementsToShow;
        ++Index)
    {
        u32 AssetIndex = Cursor->Array[Index];
        switch(Cursor->DataType)
        {
            case CursorDataType_StringArray:
            {
                char *Text = Cursor->StringArray[AssetIndex];
                BasicTextElement(Layout, Text, ItemInteraction, V4(0.8f, 0.8f, 0.8f, 1), V4(1, 1, 1, 1),
                                 3.0f, V4(0, 0, 1, 1.0f));
            } break;
        }
    }

    return(SourceArrayIndex);
}

internal void
EndUI(ui_context *UIContext, game_input *Input)
{
    render_group *RenderGroup = &UIContext->RenderGroup;

    UIContext->AltUI = Input->MouseButtons[1].EndedDown;
    v2 MouseP = Unproject(RenderGroup, DefaultFlatTransform(), V2i(Input->MouseX, Input->MouseY)).xy;
    UIContext->MouseTextLayout = BeginLayout(UIContext, MouseP, MouseP);
    EndLayout(&UIContext->MouseTextLayout);
    Interact(UIContext, Input, MouseP);

    
    EndRenderGroup(&UIContext->RenderGroup);

    // NOTE(casey): Clear the UI state for the next frame
    ZeroStruct(UIContext->NextHotInteraction);
}
