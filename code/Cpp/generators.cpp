/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

i32
RandomNumber(i32 Min, i32 Max)
{
    i32 Result = 0;
    srandom((u32)time(NULL));

    Result = (random() % (Max - Min + 1)) + Min;
    
    return(Result);
}

void
TreeGenerator(u32 *TileMap, i32 TreeCount, i32 TreeGap, i32 MapWidth, i32 MapHeight)
{
    i32 TileIndex = 0;
    for(i32 TileY = 0;
        TileY < MapHeight;
        TileY += TreeGap)
    {
        for(i32 TileX = 0;
            TileX < MapWidth;
            TileX += TreeGap)
        {
            TileIndex = TileY * MapWidth + TileX;
            if(TileMap[TileIndex] == 0x00020000)
            {
                TileMap[TileIndex] = 0x00050000;
            }
        }
    }
}


