/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Paul Solodrai $
   ======================================================================== */

#include "generators.h"


i32
RandomNumber(i32 Min, i32 Max)
{
    i32 Result = 0;

    Result = (rand() % (Max - Min + 1)) + Min;
    
    return(Result);
}

void
TreeGenerator(u32 *TileMap, i32 TreeCount, i32 TreeGap,
              i32 MapWidth, i32 MapHeight, i32 TreeChance)
{
    i32 TreeCounter = 0;
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
            if((TileMap[TileIndex] == 0x00020000) && (TreeChance >= RandomNumber(1, 100)))
            {
                i32 TreeIndex = RandomNumber(0, 2);
                TileMap[TileIndex] = Trees[TreeIndex];
                --TreeCount;
                ++TreeCounter;
            }
            if(TreeCount == 0)
            {
                break;
            }
        }
        if(TreeCount == 0)
        {
            break;
        }
    }
    printf("Trees in the map: %d\n", TreeCounter);
}

void
BushGenerator(u32 *TileMap, i32 BushCount, i32 BushGap,
              i32 MapWidth, i32 MapHeight, i32 BushChance)
{
    i32 BushCounter = 0;
    i32 TileIndex = 0;
    for(i32 TileY = 0;
        TileY < MapHeight;
        TileY += BushGap)
    {
        for(i32 TileX = 0;
            TileX < MapWidth;
            TileX += BushGap)
        {
            TileIndex = TileY * MapWidth + TileX;
            if((TileMap[TileIndex] == 0x00020000) && (BushChance >= RandomNumber(1, 100)))
            {
                //i32 BushIndex = RandomNumber(0, 2);
                TileMap[TileIndex] = Bushes;
                --BushCount;
                ++BushCounter;
            }
            if(BushCount == 0)
            {
                break;
            }
        }
        if(BushCount == 0)
        {
            break;
        }
    }
    printf("Bushs in the map: %d\n", BushCounter);
}


