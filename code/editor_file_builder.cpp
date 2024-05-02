/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

internal loaded_bitmap
LoadBMP(char *FileName)
{
    loaded_bitmap Result = {};
    
    read_file_result ReadResult = Platform.ReadEntireFileA(FileName);    
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.WidthOverHeight = (r32)Header->Width/(r32)Header->Height;
        
        Assert(Result.Height >= 0);
        Assert(Header->Compression == 3);

        // NOTE(casey): If you are using this generically for some reason,
        // please remember that BMP files CAN GO IN EITHER DIRECTION and
        // the height will be negative for top-down.
        // (Also, there can be compression, etc., etc... DON'T think this
        // is complete BMP loading code because it isn't!!)

        // NOTE(casey): Byte order in memory is determined by the Header itself,
        // so we have to read out the masks and convert the pixels ourselves.
        uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);        
        
        bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
        
        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);

        int32 RedShiftDown = (int32)RedScan.Index;
        int32 GreenShiftDown = (int32)GreenScan.Index;
        int32 BlueShiftDown = (int32)BlueScan.Index;
        int32 AlphaShiftDown = (int32)AlphaScan.Index;
        
        uint32 *SourceDest = Pixels;
        for(int32 Y = 0;
            Y < Header->Height;
            ++Y)
        {
            for(int32 X = 0;
                X < Header->Width;
                ++X)
            {
                uint32 C = *SourceDest;

                v4 Texel  =
                    {
                        (real32)((C & RedMask) >> RedShiftDown),
                        (real32)((C & GreenMask) >> GreenShiftDown),
                        (real32)((C & BlueMask) >> BlueShiftDown),
                        (real32)((C & AlphaMask) >> AlphaShiftDown)
                    };

                Texel = SRGB255ToLinear1(Texel);

#if 1
                Texel.rgb *= Texel.a;
#endif
                Texel = Linear1ToSRGB255(Texel);
                
                *SourceDest++ = (((uint32)(Texel.a + 0.5f) << 24) |
                                 ((uint32)(Texel.r + 0.5f) << 16) |
                                 ((uint32)(Texel.g + 0.5f) << 8) |
                                 ((uint32)(Texel.b + 0.5f) << 0));
            }
        }
    }

    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
#if 0
    Result.Memory = (uint8 *)Result.Memory + Result.Pitch*(Result.Height - 1);
    Result.Pitch = -Result.Pitch;
#endif
    
    return(Result);
}
