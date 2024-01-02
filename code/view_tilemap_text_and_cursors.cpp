/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

#include <stdio.h>

global_variable r32 LeftEdge;
global_variable r32 AtY;
global_variable r32 FontScale;
global_variable font_id FontID;

internal void
DEBUGReset(render_group *RenderGroup, s32 Width, s32 Height)
{
    asset_vector MatchVector = {};
    asset_vector WeightVector = {};

    MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
    WeightVector.E[Tag_FontType] = 1.0f;
    
    FontID = GetBestMatchFontFrom(RenderGroup->Assets, Asset_Font, &MatchVector, &WeightVector);

    FontScale = 1.0f;
    AtY = 0.0f;
    LeftEdge = -0.5f*Width;

    ssa_font *Info = GetFontInfo(RenderGroup->Assets, FontID);
    AtY = 0.5f*Height - FontScale*GetStartingBaselineY(Info);
}

internal void
DEBUGTextLine(render_group *RenderGroup, char *String)
{    
    loaded_font *Font = PushFont(RenderGroup, FontID);
    if(Font)
    {
        ssa_font *FontInfo = GetFontInfo(RenderGroup->Assets, FontID);
            
        u32 PrevCodePoint = 0;
        r32 CharScale = FontScale;
        v4 Color = V4(0, 0, 0, 1);
        r32 AtX = LeftEdge;
        for(char *At = String;
            *At;
            )
        {
            u32 CodePoint = *At;
            r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(FontInfo, Font, PrevCodePoint, CodePoint);
            AtX += AdvanceX;

            if(CodePoint != ' ')
            {
                bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, FontInfo, Font, CodePoint);
                ssa_bitmap *Info = GetBitmapInfo(RenderGroup->Assets, BitmapID);
                PushBitmap(RenderGroup, BitmapID, CharScale*(r32)Info->Dim[1], V3(AtX, AtY, 0), Color);
            }
            PrevCodePoint = CodePoint;
                
            ++At;
        }
        AtY -= GetLineAdvanceFor(FontInfo)*FontScale;
    }
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
InitializeCursor(memory_arena *Arena, array_cursor *Cursor, u32 CursorArrayCount)
{
    Cursor->ArrayPosition = 0;
    Cursor->ArrayCount = CursorArrayCount;
    Cursor->Array = PushArray(Arena, Cursor->ArrayCount, u32);
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
