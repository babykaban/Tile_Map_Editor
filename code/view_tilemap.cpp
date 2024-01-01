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
global_variable tileset_id TilesetID; 
global_variable loaded_tileset *GlobalTileset; 
global_variable ssa_tileset *GlobalTilesetInfo;

inline tile_position
TilePositionFromChunkPosition(world_position *Pos)
{
    tile_position Result = {};

    u32 HalfChunkCount = TILES_PER_CHUNK / 2;
    Result.TileX = Pos->ChunkX*TILES_PER_CHUNK + HalfChunkCount;
    Result.TileY = Pos->ChunkY*TILES_PER_CHUNK + HalfChunkCount; 

    if(Pos->Offset_.x > 0.0f)
    {
        Result.TileX += CeilReal32ToInt32(Pos->Offset_.x) - 1;
    }
    else
    {
        Result.TileX += FloorReal32ToInt32(Pos->Offset_.x);
    }

    if(Pos->Offset_.y > 0.0f)
    {
        Result.TileY += CeilReal32ToInt32(Pos->Offset_.y) - 1;
    }
    else
    {
        Result.TileY += FloorReal32ToInt32(Pos->Offset_.y);
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
WriteMap(char *FileName, u32 TileCount, u32 *TileIDs)
{
    uint32 ContentSize = sizeof(u32)*TileCount;
    Platform.DEBUGWriteEntireFile(0, FileName, ContentSize, TileIDs);
}

internal void
WriteWorldTiles(char *FileName, u32 TileCount, world_tile *WorldTiles)
{
    uint32 ContentSize = sizeof(world_tile)*TileCount;
    Platform.DEBUGWriteEntireFile(0, FileName, ContentSize, WorldTiles);
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

internal void
FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer,
                world_position *ChunkP, real32 TileSideInMeters)
{
    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);

    loaded_bitmap *Buffer = &GroundBuffer->Bitmap;
    Buffer->AlignPercentage = V2(0.5f, 0.5f);
    Buffer->WidthOverHeight = 1.0f; 

    real32 Width = GameState->World->ChunkDimInMeters.x;
    real32 Height = GameState->World->ChunkDimInMeters.y;
    Assert(Width == Height);
    v2 HalfDim = 0.5f*V2(Width, Height);

    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4),
                                                    true, true);
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
//                if((TileX < 8) && (TileY < 8))
                {
                    world_tile *WorldTile = GameState->WorldTiles + TileY*TileCountX + TileX;

                    real32 X = (real32)(TileX - MinTileX);
                    real32 Y = (real32)(TileY - MinTileY);
                    v2 P = -HalfDim + V2(TileSideInMeters*X, TileSideInMeters*Y);
                    
                    bitmap_id ID = WorldTile->TileBitmapID;
                    PushBitmap(RenderGroup, ID, 1.0f, V3(P, 0.0f));
                }
            }
        }
    }

    TiledRenderGroupToOutput(TranState->LowPriorityQueue, RenderGroup, Buffer);
    EndRender(RenderGroup);
    EndTemporaryMemory(GroundMemory);
}

internal void
InitializeWorldTiles(game_assets *Assets, game_state *GameState, char *FileName)
{
    debug_read_file_result WorldTiles = Platform.DEBUGReadEntireFile(0, FileName);
    if(WorldTiles.Contents)
    {
        GameState->WorldTiles = (world_tile *)WorldTiles.Contents;
        for(u32 TileIndex = 0;
            TileIndex < GameState->WorldTileCount;
            ++TileIndex)
        {
            world_tile *Tile = GameState->WorldTiles + TileIndex;
            GameState->TileIDs[TileIndex] = Tile->TileID;
            Tile->TileBitmapID = GetBitmapForTileID(GlobalTileset, GlobalTilesetInfo, Tile->TileID,
                                                    GameState->GlobalTilesetID);
        }
    }
    else
    {
        asset_vector MatchVector = {};
        asset_vector WeightVector = {};
        WeightVector.E[Tag_TileID] = 1.0f;
        MatchVector.E[Tag_TileID] = 3.0f;
        bitmap_id TileBitmapID = GetBestMatchBitmapFrom(Assets, Asset_Tile,
                                                        &MatchVector, &WeightVector);
        for(u32 TileIndex = 0;
            TileIndex < GameState->WorldTileCount;
            ++TileIndex)
        {
            world_tile *WorldTile = GameState->WorldTiles + TileIndex;
            GameState->TileIDs[TileIndex] = 3;
            WorldTile->TileID = 3;
            WorldTile->TileBitmapID = TileBitmapID;
        }
    }

    GameState->WorldTilesInitialized = true;
}

internal u32
GetTileIndexFromChunkPosition(game_state *GameState, world_position *MouseP)
{
    u32 Result = 0;
    tile_position MouseTileP = TilePositionFromChunkPosition(MouseP);
    u32 WorldTileIndex = MouseTileP.TileY*WORLD_HEIGHT_TILE_COUNT + MouseTileP.TileX;

    Result = WorldTileIndex;
    
    return(Result);
}

inline world_tile *
GetTileFromChunkPosition(game_state *GameState, world_position *MouseP)
{
    world_tile *Result = 0;
    u32 WorldTileIndex = GetTileIndexFromChunkPosition(GameState, MouseP);

    Result = GameState->WorldTiles + WorldTileIndex;
    
    return(Result);
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

internal void
ChangeTile(render_group *RenderGroup, game_state *GameState, world_position *MouseP)
{
    u32 TileToChange = GetTileIndexFromChunkPosition(GameState, MouseP);
    if((MouseP->ChunkX >= 0) && (MouseP->ChunkY >= 0))
    {
        loaded_tileset *Tileset = PushTileset(RenderGroup, TilesetID);
        if(Tileset)
        {
            u32 TileIndex = GameState->MenuBarCursor.Array[GameState->MenuBarCursor.ArrayPosition];
            ssa_tile *Tile = Tileset->Tiles + TileIndex; 
            GameState->WorldTiles[TileToChange].TileID = Tile->UniqueID;
            GameState->WorldTiles[TileToChange].TileBitmapID.Value = Tile->BitmapID.Value + Tileset->BitmapIDOffset;

            GameState->TileIDs[TileToChange] = Tile->UniqueID;

            ResetGroundBuffers(RenderGroup->Assets->TranState);
        }
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

internal void
ShowTileMenuBar(render_group *RenderGroup, array_cursor *Cursor, r32 TileDimInMeters)
{
    loaded_tileset *Tileset = PushTileset(RenderGroup, TilesetID);
    u32 TileCountInBar = Cursor->ArrayCount;

    r32 TileHalfDim = 0.5f*TileDimInMeters;
    r32 MenuBarWidth = TileDimInMeters*TileCountInBar;
    r32 HalfMenuBarWidth = 0.5f*MenuBarWidth;
    r32 OffsetY = 6.0f;
    PushRectOutline(RenderGroup, V3(0, OffsetY, 0), V2(MenuBarWidth, TileDimInMeters), V4(0, 1, 0, 1), 0.05f);

    ssa_tileset *Info = GetTilesetInfo(RenderGroup->Assets, TilesetID);
    if(Tileset)
    {
        r32 TileOffsetX = -HalfMenuBarWidth;
        r32 TileOffsetY = OffsetY - TileHalfDim;
        for(u32 Index = 0;
            Index < TileCountInBar;
            ++Index)
        {
            u32 TileIndex = Cursor->Array[Index];
            bitmap_id ID = GetBitmapForTile(RenderGroup->Assets, Info, Tileset, TileIndex);
            PushBitmap(RenderGroup, ID, TileDimInMeters, V3(TileOffsetX, TileOffsetY, 0));
            TileOffsetX += TileDimInMeters;
        }
    }

    r32 CursorOffsetX = (TileHalfDim - HalfMenuBarWidth) + Cursor->ArrayPosition*TileDimInMeters;
    PushRectOutline(RenderGroup, V3(CursorOffsetX, OffsetY, 0), V2(TileDimInMeters, TileDimInMeters), V4(1, 1, 1, 1), 0.05f);
}

internal void
ShowTilesetStats(render_group *RenderGroup, game_state *GameState)
{
    DEBUGTextLine(RenderGroup, "Tileset Stats:");
    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer), "Biome: %s", Biomes[GameState->SetStats.Biome]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "TileType: %s", TileTypes[GameState->SetStats.Type]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "Height: %s", Heights[GameState->SetStats.Height]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "CliffHillType: %s",
                CliffHillTypes[GameState->SetStats.CliffHillType]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "MainSurface: %s", Surfaces[GameState->SetStats.MainSurface]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer), "MergeSurface: %s", Surfaces[GameState->SetStats.MergeSurface]);
    DEBUGTextLine(RenderGroup, TextBuffer);
}

internal void
ReloadTileset(game_assets *Assets, game_state *GameState)
{
    asset_vector WeightVector = {};
    WeightVector.E[Tag_BiomeType] = 1.0f;
    WeightVector.E[Tag_TileType] = 1.0f;
    WeightVector.E[Tag_Height] = 1.0f;
    WeightVector.E[Tag_CliffHillType] = 1.0f;
    WeightVector.E[Tag_TileMainSurface] = 1.0f;
    WeightVector.E[Tag_TileMergeSurface] = 1.0f;

    asset_vector MatchVector = {};
    MatchVector.E[Tag_BiomeType] = (r32)GameState->SetStats.Biome;
    MatchVector.E[Tag_TileType] = (r32)GameState->SetStats.Type;
    MatchVector.E[Tag_Height] = (r32)GameState->SetStats.Height;
    MatchVector.E[Tag_CliffHillType] = (r32)GameState->SetStats.CliffHillType;
    MatchVector.E[Tag_TileMainSurface] = (r32)GameState->SetStats.MainSurface;
    MatchVector.E[Tag_TileMergeSurface] = (r32)GameState->SetStats.MergeSurface;
    
    tileset_id ID = GetBestMatchTilesetFrom(Assets, Asset_Tileset, &MatchVector, &WeightVector);
    if(ID.Value != TilesetID.Value)
    {
        TilesetID = ID;
        ResetCursorArray(&GameState->MenuBarCursor);
    }
}

internal void
ShowTest(render_group *RenderGroup, array_cursor *Cursor)
{
    DEBUGTextLine(RenderGroup, "Test:");
    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
                Cursor->Array[0], Cursor->Array[1], Cursor->Array[2], Cursor->Array[3],
                Cursor->Array[4], Cursor->Array[5], Cursor->Array[6], Cursor->Array[7],
                Cursor->Array[8], Cursor->Array[9]);
    DEBUGTextLine(RenderGroup, TextBuffer);
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

        v2 WorldChunkDimInMeters =
            {
                TILES_PER_CHUNK * TileSideInMeters,
                TILES_PER_CHUNK * TileSideInMeters
            };

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));


        GameState->WorldTileCount = WORLD_HEIGHT_TILE_COUNT*WORLD_WIDTH_TILE_COUNT;
        GameState->WorldTiles = PushArray(&GameState->WorldArena, GameState->WorldTileCount, world_tile);
        GameState->TileIDs = PushArray(&GameState->WorldArena, GameState->WorldTileCount, u32);
        
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

        GameState->SetStats = {};
        
        InitializeCursor(&GameState->WorldArena, &GameState->MenuBarCursor, 10);
        
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

        asset_vector MatchVector = {};
        asset_vector WeightVector = {};
        WeightVector.E[Tag_BiomeType] = 1.0f;
        WeightVector.E[Tag_TileMainSurface] = 1.0f;
        WeightVector.E[Tag_TileMergeSurface] = 1.0f;

        MatchVector.E[Tag_BiomeType] = (r32)BiomeType_Global;
        MatchVector.E[Tag_TileMainSurface] = (r32)TileSurface_Global;
        MatchVector.E[Tag_TileMergeSurface] = (r32)TileSurface_Global;
        tileset_id ID = GetBestMatchTilesetFrom(TranState->Assets, Asset_Tileset,
                                                &MatchVector, &WeightVector);

        GameState->GlobalTilesetID = ID;
        
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
        
        GameState->CameraP = MapIntoChunkSpace(World, GameState->CameraP, NewP);

        if(Controller->Biome.EndedDown)
        {
            GameState->SetStats.Biome += 1;
            if(GameState->SetStats.Biome >= BiomeType_Count)
            {
                GameState->SetStats.Biome = 0;
            }
        }

        if(Controller->Type.EndedDown)
        {
            GameState->SetStats.Type += 1;
            if(GameState->SetStats.Type >= TileType_Count)
            {
                GameState->SetStats.Type = 0;
            }
        }

        if(Controller->Height.EndedDown)
        {
            GameState->SetStats.Height += 1;
            if(GameState->SetStats.Height >= Height_Count)
            {
                GameState->SetStats.Height = 0;
            }
        }

        if(Controller->CliffHillType.EndedDown)
        {
            GameState->SetStats.CliffHillType += 1;
            if(GameState->SetStats.CliffHillType >= CliffHillType_Count)
            {
                GameState->SetStats.CliffHillType = 0;
            }
        }

        if(Controller->MainSurface.EndedDown)
        {
            GameState->SetStats.MainSurface += 1;
            if(GameState->SetStats.MainSurface >= TileSurface_Count)
            {
                GameState->SetStats.MainSurface = 0;
            }
        }
        if(Controller->MergeSurface.EndedDown)
        {
            GameState->SetStats.MergeSurface += 1;
            if(GameState->SetStats.MergeSurface >= TileSurface_Count)
            {
                GameState->SetStats.MergeSurface = 0;
            }
        }

        if(Controller->ActionUp.EndedDown)
        {
//            ChangeChoosenAttributeValueFor(GameState, true);
        }
        if(Controller->ActionDown.EndedDown)
        {
//            ChangeChoosenAttributeValueFor(GameState, false);
        }
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

    render_group *TextRenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false, false);
    BeginRender(TextRenderGroup);
    Ortographic(TextRenderGroup, Buffer->Width, Buffer->Height, 1.0f);

    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false, false);
    BeginRender(RenderGroup);
    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;// / 2.0f;
    real32 FocalLength = 0.6f;
    real32 DistanceAboveTarget = 9.0f;
    
    Prespective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, FocalLength, DistanceAboveTarget);


    GlobalTileset = PushTileset(RenderGroup, GameState->GlobalTilesetID, true);
    GlobalTilesetInfo = GetTilesetInfo(TranState->Assets, GameState->GlobalTilesetID);
    if(!GameState->WorldTilesInitialized)
    {
        InitializeWorldTiles(TranState->Assets, GameState, "worldtiles.bin");
    }

    
    RenderGroup->Transform.OffsetP = V3(0, 0, 0);
    
    // NOTE(paul): Reset font spacing
    DEBUGReset(RenderGroup, Buffer->Width, Buffer->Height);

    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

    v2 ScreenCenter = {0.5f*(real32)DrawBuffer->Width,
                       0.5f*(real32)DrawBuffer->Height};

    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
    rectangle2 CameraBoundsInMeters = RectMinMax(ScreenBounds.Min, ScreenBounds.Max);
    
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
    
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "MouseP in Chunks: X: %d Y: %d, OX: %f, OY: %f",
                MouseChunkP.ChunkX, MouseChunkP.ChunkY,
                MouseChunkP.Offset_.x, MouseChunkP.Offset_.y);
    DEBUGTextLine(TextRenderGroup, TextBuffer);

    tile_position MouseTileP = TilePositionFromChunkPosition(&MouseChunkP);
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "MouseP in Tiles: X: %d Y: %d",
                MouseTileP.TileX, MouseTileP.TileY);
    DEBUGTextLine(TextRenderGroup, TextBuffer);

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

            PushRectOutline(RenderGroup, V3(Delta, 0), V2(GroundSideInMeters, GroundSideInMeters),
                            V4(1.0f, 1.0f, 0.0f, 1.0f), 0.01f);
        }
    }

    if(Input->MouseButtons[0].EndedDown)
    {
        ChangeTile(RenderGroup, GameState, &MouseChunkP);
    }
    
    ssa_tileset *Info = GetTilesetInfo(RenderGroup->Assets, TilesetID);
    ChangeCursorPositionFor(&GameState->MenuBarCursor, Info->TileCount, Input->MouseZ);
    ShowTest(TextRenderGroup, &GameState->MenuBarCursor);
    ShowTileMenuBar(RenderGroup, &GameState->MenuBarCursor, TileSideInMeters);
    ShowTilesetStats(TextRenderGroup, GameState);
    
    WriteMap("tilemap.bin", GameState->WorldTileCount, GameState->TileIDs);
    WriteWorldTiles("worldtiles.bin", GameState->WorldTileCount, GameState->WorldTiles);
    ReloadTileset(RenderGroup->Assets, GameState);
    
    PushRectOutline(RenderGroup, V3(0, 0, 0), GetDim(ScreenBounds), V4(1.0f, 1.0f, 0.0f, 1));
    PushRect(RenderGroup, V3(0, 0, 0), 0.2f*V2(TileSideInMeters, TileSideInMeters), V4(1, 0, 0, 1));

    v2 Delta = Subtract(World, &GameState->CameraP, &MouseChunkP);
    PushRect(RenderGroup, V3(-Delta, 0),
             0.2f*V2(TileSideInMeters, TileSideInMeters), V4(0, 0, 1, 1));

    tile_position Tp = TilePositionFromChunkPosition(&MouseChunkP);
    tile_position TCp = TilePositionFromChunkPosition(&GameState->CameraP);
    

    v2 dTile =
        {
            (real32)TCp.TileX - (real32)Tp.TileX,
            (real32)TCp.TileY - (real32)Tp.TileY
        };

    v2 D = dTile*TileSideInMeters - V2(0.5f, 0.5f);

    PushRectOutline(RenderGroup, V3(-D, 0), V2(TileSideInMeters, TileSideInMeters),
                    V4(0.0f, 0.0f, 1.0f, 1), 0.0125f);
    
    RenderGroup->GlobalAlpha = 1.0f;

    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer);
    EndRender(RenderGroup);
    
    EndTemporaryMemory(RenderMemory);
    
    CheckArena(&GameState->WorldArena);
    CheckArena(&TranState->TranArena);

    TiledRenderGroupToOutput(TranState->HighPriorityQueue, TextRenderGroup, DrawBuffer);
    EndRender(TextRenderGroup);
}



