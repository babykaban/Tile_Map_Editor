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
    object_transform NoTransform = DefaultFlatTransform();
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
                NoTransform.SortBias = 10000.0f;
                PushBitmap(RenderGroup, NoTransform, BitmapID, CharScale*(r32)Info->Dim[1], V3(AtX, AtY, 0), Color);
            }
            PrevCodePoint = CodePoint;
                
            ++At;
        }
        AtY -= GetLineAdvanceFor(FontInfo)*FontScale;
    }
}
