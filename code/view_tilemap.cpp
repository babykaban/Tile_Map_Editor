/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Paul Solodrai $
   ======================================================================== */

#include "view_tilemap.h"
#include "view_tilemap_render_group.cpp"
#include "view_tilemap_world.cpp"
#include "view_tilemap_sim_region.cpp"
#include "view_tilemap_asset.cpp"

#include <stdio.h>
global_variable r32 LeftEdge;
global_variable r32 AtY;
global_variable r32 FontScale;
global_variable font_id FontID;

inline tile_position
TilePositionFromChunkPosition(world_position Pos)
{
    tile_position Result = {};

    u32 HalfChunkCount = TILES_PER_CHUNK / 2;
    Result.TileX = Pos.ChunkX*TILES_PER_CHUNK + HalfChunkCount;
    Result.TileY = Pos.ChunkY*TILES_PER_CHUNK + HalfChunkCount; 

    if(Pos.Offset_.x > 0.0f)
    {
        Result.TileX += CeilReal32ToInt32(Pos.Offset_.x) - 1;
    }
    else
    {
        Result.TileX += FloorReal32ToInt32(Pos.Offset_.x);
    }

    if(Pos.Offset_.y > 0.0f)
    {
        Result.TileY += CeilReal32ToInt32(Pos.Offset_.y) - 1;
    }
    else
    {
        Result.TileY += FloorReal32ToInt32(Pos.Offset_.y);
    }

    return(Result);
}

inline world_position
ChunkPositionFromTilePosition(world *World, int32 AbsTileX, int32 AbsTileY,
                              v2 AdditionalOffset = V2(0, 0))
{
    world_position BasePos = {};

    real32 TileSideInMeters = 1.0f;
    
    v2 TileDim = V2(TileSideInMeters, TileSideInMeters);
    v2 Offset = Hadamard(TileDim, V2((real32)AbsTileX, (real32)AbsTileY));
    world_position Result = MapIntoChunkSpace(World, BasePos, AdditionalOffset + Offset);
    
    Assert(IsCanonical(World, Result.Offset_));
    
    return(Result);
}

internal void
ClearBitmap(loaded_bitmap *Bitmap)
{
    if(Bitmap->Memory)
    {
        int32 TotalBitmapSize = Bitmap->Width*Bitmap->Height*BITMAP_BYTES_PER_PIXEL;
        ZeroSize(TotalBitmapSize, Bitmap->Memory);
    }
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *Arena, int32 Width, int32 Height, bool32 ClearToZero = true)
{
    loaded_bitmap Result = {};

    Result.Width = Width;
    Result.Height = Height;
    Result.AlignPercentage = V2(0.5f, 0.5f);
    Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);
    
    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
    int32 TotalBitmapSize = Width*Height*BITMAP_BYTES_PER_PIXEL;
    Result.Memory = PushSize(Arena, TotalBitmapSize, 16);
    if(ClearToZero)
    {
        ClearBitmap(&Result);
    }

    return(Result);
}

internal task_with_memory *
BeginTaskWithMemory(transient_state *TranState)
{
    task_with_memory *FoundTask = 0;
    for(uint32 TaskIndex = 0;
        TaskIndex < ArrayCount(TranState->Tasks);
        ++TaskIndex)
    {
        task_with_memory *Task = TranState->Tasks + TaskIndex;
        if(!Task->BeingUsed)
        {
            Task->BeingUsed = true;
            FoundTask = Task;
            Task->MemoryFlush = BeginTemporaryMemory(&Task->Arena);
            break;
        }
    }

    return(FoundTask);
}

internal void
EndTaskWithMemory(task_with_memory *Task)
{
    EndTemporaryMemory(Task->MemoryFlush);

    CompletePreviousWritesBeforeFutureWrites;
    Task->BeingUsed = false;
}

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

#if 0
internal void
FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer,
                world_position *ChunkP, real32 TileSideInMeters)
{
    random_series Series = RandomSeed(236346);
    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);

    loaded_bitmap *Buffer = &GroundBuffer->Bitmap;
    Buffer->AlignPercentage = V2(0.5f, 0.5f);
    Buffer->WidthOverHeight = 1.0f; 

    real32 Width = GameState->World->ChunkDimInMeters.x;
    real32 Height = GameState->World->ChunkDimInMeters.y;
    Assert(Width == Height);
    v2 HalfDim = 0.5f*V2(Width, Height);

    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), true);
    BeginRender(RenderGroup);
    Ortographic(RenderGroup, Buffer->Width, Buffer->Height, Buffer->Width / Width);
    Clear(RenderGroup, V4(1.0f, 0.0f, 1.0f, 1.0f));

    GroundBuffer->P = *ChunkP;

    int32 TileCountX = WORLD_WIDTH_TILE_COUNT;
    int32 TileCountY = WORLD_HEIGHT_TILE_COUNT;

    if((ChunkP->ChunkX >= 0) && (ChunkP->ChunkY >= 0))
    {
        int32 MinTileX = TILES_PER_CHUNK*ChunkP->ChunkX;
        int32 MaxTileX = MinTileX + TILES_PER_CHUNK;
        int32 MinTileY = TILES_PER_CHUNK*ChunkP->ChunkY;
        int32 MaxTileY = MinTileY + TILES_PER_CHUNK;

        for(int32 TileY = MinTileY;
            TileY < MaxTileY;
            ++TileY)
        {
            for(int32 TileX = MinTileX;
                TileX < MaxTileX;
                ++TileX)
            {
                if((TileX < TileCountX) && (TileY < TileCountY))
                {
                    world_tile *WorldTile = GameState->WorldTiles + TileY*TileCountX + TileX;

                    real32 X = (real32)(TileX - MinTileX);
                    real32 Y = (real32)(TileY - MinTileY);
                    v2 P = -HalfDim + V2(TileSideInMeters*X, TileSideInMeters*Y);
                    
                    bitmap_id ID = GetFirstBitmapFrom(TranState->Assets, Asset_TestTile);
                    PushBitmap(RenderGroup, ID, TileSideInMeters, V3(P, 0.0f));
                }
            }
        }
    }

    TiledRenderGroupToOutput(TranState->LowPriorityQueue, RenderGroup, Buffer);
    EndRender(RenderGroup);
    EndTemporaryMemory(GroundMemory);
}
#else

internal void
FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer,
                world_position *ChunkP, real32 TileSideInMeters)
{
    random_series Series = RandomSeed(236346);
    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);

    loaded_bitmap *Buffer = &GroundBuffer->Bitmap;
    Buffer->AlignPercentage = V2(0.5f, 0.5f);
    Buffer->WidthOverHeight = 1.0f; 

    real32 Width = GameState->World->ChunkDimInMeters.x;
    real32 Height = GameState->World->ChunkDimInMeters.y;
    Assert(Width == Height);
    v2 HalfDim = 0.5f*V2(Width, Height);

    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), true);
    BeginRender(RenderGroup);
    Ortographic(RenderGroup, Buffer->Width, Buffer->Height, Buffer->Width / Width);
    Clear(RenderGroup, V4(1.0f, 0.0f, 1.0f, 1.0f));

    GroundBuffer->P = *ChunkP;

    int32 TileCountX = WORLD_WIDTH_TILE_COUNT;
    int32 TileCountY = WORLD_HEIGHT_TILE_COUNT;

    if((ChunkP->ChunkX >= 0) && (ChunkP->ChunkY >= 0))
    {
        int32 MinTileX = TILES_PER_CHUNK*ChunkP->ChunkX;
        int32 MaxTileX = MinTileX + TILES_PER_CHUNK;
        int32 MinTileY = TILES_PER_CHUNK*ChunkP->ChunkY;
        int32 MaxTileY = MinTileY + TILES_PER_CHUNK;

        for(int32 TileY = MinTileY;
            TileY < MaxTileY;
            ++TileY)
        {
            for(int32 TileX = MinTileX;
                TileX < MaxTileX;
                ++TileX)
            {
                if((TileX < TileCountX) && (TileY < TileCountY))
                {
                    world_tile *WorldTile = GameState->WorldTiles + TileY*TileCountX + TileX;

                    real32 X = (real32)(TileX - MinTileX);
                    real32 Y = (real32)(TileY - MinTileY);
                    v2 P = -HalfDim + V2(TileSideInMeters*X, TileSideInMeters*Y);
                    
                    bitmap_id ID = GetFirstBitmapFrom(TranState->Assets, Asset_TestTile);
                    PushBitmap(RenderGroup, ID, 1.0f, V3(P, 0.0f));
                }
            }
        }
    }


//    RenderGroupToOutput(RenderGroup, Buffer);
    TiledRenderGroupToOutput(TranState->LowPriorityQueue, RenderGroup, Buffer);
    EndRender(RenderGroup);
    EndTemporaryMemory(GroundMemory);
}
#endif

internal world_tile *
GetTileFromChunkPosition(game_state *GameState, world_position MouseP)
{
    world_tile *Result = 0;
    tile_position MouseTileP = TilePositionFromChunkPosition(MouseP);

    u32 WorldTileIndex = MouseTileP.TileY*WORLD_HEIGHT_TILE_COUNT + MouseTileP.TileY*WORLD_WIDTH_TILE_COUNT;

    Result = GameState->WorldTiles + WorldTileIndex;
    
    return(Result);
}

internal void
ShowTile(render_group *RenderGroup, game_state *GameState, world_position MouseChunkP)
{
    if((MouseChunkP.ChunkX >= 0) && (MouseChunkP.ChunkY >= 0))
    {
        r32 TileSize = 100.0f;
        world_tile *WorldTile = GetTileFromChunkPosition(GameState, MouseChunkP);
        PushBitmap(RenderGroup, WorldTile->TileBitmapID, TileSize, V3(0, 0, 0));
    }
}

internal void
InitializeWorldTiles(game_assets *Assets, game_state *GameState, u32 *ReferenceTiles)
{
    if(ReferenceTiles)
    {
        // TODO(paul): MakeReferenceArray
    }
    else
    {
        asset_vector MatchVector = {};
        MatchVector.E[Tag_TileBiomeType] = BiomeType_AncientForest;
        MatchVector.E[Tag_TileState] = TileState_Solid;
        MatchVector.E[Tag_TileMainSurface] = TileSurface_0;
        MatchVector.E[Tag_TileMergeSurface] = TileSurface_0;

        asset_vector WeightVector = {};
        WeightVector.E[Tag_TileBiomeType] = 1.0f;
        WeightVector.E[Tag_TileState] = 1.0f;
        WeightVector.E[Tag_TileMainSurface] = 1.0f;
        WeightVector.E[Tag_TileMergeSurface] = 1.0f;

        bitmap_id TileBitmapID = GetBestMatchBitmapFrom(Assets, Asset_Tile,
                                                        &MatchVector, &WeightVector);

        for(u32 TileIndex = 0;
            TileIndex < GameState->WorldTileCount;
            ++TileIndex)
        {
            world_tile *WorldTile = GameState->WorldTiles + TileIndex;
            WorldTile->TileBitmapID = TileBitmapID;
            WorldTile->MatchVector = MatchVector;
            WorldTile->WeightVector = WeightVector;
        }
    }
}

inline void
ResetGroundBuffers(transient_state *TranState)
{
    for(uint32 GroundBufferIndex = 0;
        GroundBufferIndex < TranState->GroundBufferCount;
        ++GroundBufferIndex)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
        GroundBuffer->P = NullPosition();            
    }        
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{    
    Platform = Memory->PlatformAPI;

    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));

    uint32 GroundBufferWidth = 256; 
    uint32 GroundBufferHeight = 256;

    uint32 TileSideInPixels = 32;
    real32 PixelsToMeters = 1.0f / TileSideInPixels;
    real32 TileSideInMeters = TileSideInPixels * PixelsToMeters;

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        uint32 TilesPerWidth = 30;
        uint32 TilesPerHeight = 17;
#if 1
        v2 WorldChunkDimInMeters =
            {
                TILES_PER_CHUNK * TileSideInMeters,
                TILES_PER_CHUNK * TileSideInMeters
            };
#else
        v2 WorldChunkDimInMeters =
            {
                PixelsToMeters*GroundBufferWidth,
                PixelsToMeters*GroundBufferHeight,
            };
#endif
        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));


        GameState->BiomeMask = 0xFF000000;
        GameState->TileStateMask = 0x00FF0000;
        GameState->TileSurfaceMainMask = 0x0000FF00;
        GameState->TileSurfaceMergeMask = 0x000000FF;
        GameState->WorldTileCount = WORLD_HEIGHT_TILE_COUNT*WORLD_WIDTH_TILE_COUNT;
        GameState->WorldTiles = PushArray(&GameState->WorldArena, GameState->WorldTileCount, world_tile);
        
        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        InitializeWorld(World, WorldChunkDimInMeters);
        
        uint32 ScreenBaseX = 0;
        uint32 ScreenBaseY = 0;
        uint32 ScreenX = ScreenBaseX;
        uint32 ScreenY = ScreenBaseY;
        for(uint32 ScreenIndex = 0;
            ScreenIndex < 48;
            ++ScreenIndex)
        {
            for(uint32 TileY = 0;
                TileY < TilesPerHeight;
                ++TileY)
            {
                for(uint32 TileX = 0;
                    TileX < TilesPerWidth;
                    ++TileX)
                {
                    uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;
                }
            }

            if(ScreenX < 7)
            {
                ++ScreenX;
            }
            else
            {
                ScreenX = 0;
            }

            if((ScreenY < 5) && (ScreenX == 0))
            {
                ++ScreenY;
            }
        }
        
        world_position NewCameraP = {};
        GameState->CameraP = NewCameraP;

        GameState->CameraBoundsMin.ChunkX = 0;
        GameState->CameraBoundsMin.ChunkY = 0;
    
        GameState->CameraBoundsMax.ChunkX = WORLD_WIDTH_TILE_COUNT / TILES_PER_CHUNK;
        GameState->CameraBoundsMax.ChunkY = WORLD_HEIGHT_TILE_COUNT / TILES_PER_CHUNK;

        GameState->ViewTile = false;

        Memory->IsInitialized = true;
    }


    // NOTE(casey): Transient initialization
    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);    
    transient_state *TranState = (transient_state *)Memory->TransientStorage;
    if(!TranState->IsInitialized)
    {
        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));

        TranState->HighPriorityQueue = Memory->HighPriorityQueue;
        TranState->LowPriorityQueue = Memory->LowPriorityQueue;
        
        for(uint32 TaskIndex = 0;
            TaskIndex < ArrayCount(TranState->Tasks);
            ++TaskIndex)
        {
            task_with_memory *Task = TranState->Tasks + TaskIndex;

            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, Megabytes(1));
        }

        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(16), TranState);

        InitializeWorldTiles(TranState->Assets, GameState, 0);
        
        TranState->GroundBufferCount = 256;
        TranState->GroundBuffers = PushArray(&TranState->TranArena, TranState->GroundBufferCount, ground_buffer);
        for(uint32 GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
            GroundBuffer->P = NullPosition();
        }
        
        TranState->IsInitialized = true;
    }
    
    
#if 1
    if(Input->ExecutableReloaded)
    {
        ResetGroundBuffers(TranState);
    }
#endif    

    world *World = GameState->World;

    //
    // NOTE(casey): 
    //
    game_controller_input *Controller = GetController(Input, 0);
    if(Controller->IsAnalog)
    {
        // NOTE(casey): Use analog movement tuning
    }
    else
    {
        v2 NewP = {};
        // NOTE(casey): Use digital movement tuning
        if(Controller->MoveUp.EndedDown)
        {
            NewP.y += 4.0f;
        }
        if(Controller->MoveDown.EndedDown)
        {
            NewP.y -= 4.0f;
        }
        if(Controller->MoveLeft.EndedDown)
        {
            NewP.x -= 4.0f;
        }
        if(Controller->MoveRight.EndedDown)
        {
            NewP.x += 4.0f;
        }

        if(Input->MouseButtons[0].EndedDown)
        {
            // TODO(paul): When do I want to change tile and how
            GameState->ViewTile = true;
        }

        if(Controller->Back.EndedDown)
        {
            GameState->ViewTile = false;
        }
        
        GameState->CameraP = MapIntoChunkSpace(World, GameState->CameraP, NewP);
    }
    
    //
    // NOTE(casey): Render
    //

    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    
    loaded_bitmap DrawBuffer_ = {};
    loaded_bitmap *DrawBuffer = &DrawBuffer_;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
    DrawBuffer->Memory = Buffer->Memory;

    render_group *TextRenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false);
    BeginRender(TextRenderGroup);
    Ortographic(TextRenderGroup, Buffer->Width, Buffer->Height, 1.0f);

    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false);
    BeginRender(RenderGroup);
    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;// / 2.0f;
    real32 FocalLength = 0.6f;
    real32 DistanceAboveTarget = 9.0f;
    
    Prespective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, FocalLength, DistanceAboveTarget);

    RenderGroup->Transform.OffsetP = V3(0, 0, 0);

    // NOTE(paul): Reset font spacing
    DEBUGReset(RenderGroup, Buffer->Width, Buffer->Height);

    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

    v2 ScreenCenter = {0.5f*(real32)DrawBuffer->Width,
                       0.5f*(real32)DrawBuffer->Height};

    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
    rectangle2 CameraBoundsInMeters = RectMinMax(ScreenBounds.Min, ScreenBounds.Max);

    if(GameState->ViewTile)
    {
        DEBUGTextLine(TextRenderGroup, "ViewTile: True");
    }
    else
    {
        DEBUGTextLine(TextRenderGroup, "ViewTile: False");
    }
    
    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "CameraP in Chunks: X: %d Y: %d, OX: %f, OY: %f",
                GameState->CameraP.ChunkX, GameState->CameraP.ChunkY,
                GameState->CameraP.Offset_.x, GameState->CameraP.Offset_.y);
    DEBUGTextLine(TextRenderGroup, TextBuffer);

    r32 RenderPixelsToMeters = 1.0f / MetersToPixels;
    r32 MouseX = (Input->MouseX - ScreenCenter.x) * RenderPixelsToMeters;
    r32 MouseY = -(Input->MouseY - ScreenCenter.y) * RenderPixelsToMeters;
    
    v2 P = Unproject(RenderGroup, V2(MouseX, MouseY), 9.0f);
    world_position MouseChunkP = MapIntoChunkSpace(World, GameState->CameraP, P);

    
    tile_position MouseTileP = TilePositionFromChunkPosition(MouseChunkP);
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "MouseP in Tiles: X: %d Y: %d",
                MouseTileP.TileX, MouseTileP.TileY);
    DEBUGTextLine(TextRenderGroup, TextBuffer);
    
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "MouseP in Chunks: X: %d Y: %d, OX: %f, OY: %f",
                MouseChunkP.ChunkX, MouseChunkP.ChunkY,
                MouseChunkP.Offset_.x, MouseChunkP.Offset_.y);
    DEBUGTextLine(TextRenderGroup, TextBuffer);

    // NOTE(casey): Ground chunk rendering
    for(uint32 GroundBufferIndex = 0;
        GroundBufferIndex < TranState->GroundBufferCount;
        ++GroundBufferIndex)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
        if(IsValid(GroundBuffer->P))
        {
            loaded_bitmap *Bitmap = &GroundBuffer->Bitmap;
            v2 Delta = Subtract(GameState->World, &GroundBuffer->P, &GameState->CameraP);        

            real32 GroundSideInMeters = World->ChunkDimInMeters.x;
            PushBitmap(RenderGroup, Bitmap, GroundSideInMeters, V3(Delta, 0));

//            PushRectOutline(RenderGroup, V3(Delta, 0), V2(GroundSideInMeters, GroundSideInMeters),
//                            V4(1.0f, 1.0f, 0.0f, 1.0f), 0.02f);
        }
    }

    // NOTE(casey): Ground chunk updating
    {
        world_position MinChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMeters));
        world_position MaxChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMeters));
        for(int32 ChunkY = MinChunkP.ChunkY;
            ChunkY <= MaxChunkP.ChunkY;
            ++ChunkY)
        {
            for(int32 ChunkX = MinChunkP.ChunkX;
                ChunkX <= MaxChunkP.ChunkX;
                ++ChunkX)
            {
                world_position ChunkCenterP = CenteredChunkPoint(ChunkX, ChunkY);
                v2 RelP = Subtract(World, &ChunkCenterP, &GameState->CameraP);

                // TODO(casey): This is super inefficient fix it!
                real32 FurthestBufferLengthSq = 0.0f;
                ground_buffer *FurthestBuffer = 0;
                for(uint32 GroundBufferIndex = 0;
                    GroundBufferIndex < TranState->GroundBufferCount;
                    ++GroundBufferIndex)
                {
                    ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
                    if(AreInSameChunk(World, &GroundBuffer->P, &ChunkCenterP))
                    {
                        FurthestBuffer = 0;
                        break;
                    }
                    else if(IsValid(GroundBuffer->P))
                    {
                        RelP = Subtract(World, &GroundBuffer->P, &GameState->CameraP);
                        real32 BufferLengthSq = LengthSq(RelP);
                        if(FurthestBufferLengthSq < BufferLengthSq)
                        {
                            FurthestBufferLengthSq = BufferLengthSq;
                            FurthestBuffer = GroundBuffer;
                        }
                    }
                    else
                    {
                        FurthestBufferLengthSq = Real32Maximum;
                        FurthestBuffer = GroundBuffer;
                    }
                }

                if(FurthestBuffer)
                {
                    FillGroundChunk(TranState, GameState, FurthestBuffer, &ChunkCenterP, TileSideInMeters);
                }
            }
        }
    }

    if(GameState->ViewTile)
    {
        ShowTile(TextRenderGroup, GameState, MouseChunkP);
    }
    
    PushRectOutline(RenderGroup, V3(0, 0, 0), GetDim(ScreenBounds), V4(1.0f, 1.0f, 0.0f, 1));
    PushRect(RenderGroup, V3(0, 0, 0), 0.2f*V2(TileSideInMeters, TileSideInMeters), V4(1, 0, 0, 1));

    v2 Delta = Subtract(World, &GameState->CameraP, &MouseChunkP);
    PushRect(RenderGroup, V3(-Delta, 0),
             0.2f*V2(TileSideInMeters, TileSideInMeters), V4(0, 0, 1, 1));

    bitmap_id ID = GetFirstBitmapFrom(TranState->Assets, Asset_TestTile);
//    PushBitmap(RenderGroup, ID, 6.0f*TileSideInMeters, V3(0, 0, 0.0f));
    
    RenderGroup->GlobalAlpha = 1.0f;

    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer);
    EndRender(RenderGroup);
    
    EndTemporaryMemory(RenderMemory);
    
    CheckArena(&GameState->WorldArena);
    CheckArena(&TranState->TranArena);

    TiledRenderGroupToOutput(TranState->HighPriorityQueue, TextRenderGroup, DrawBuffer);
    EndRender(TextRenderGroup);
}



