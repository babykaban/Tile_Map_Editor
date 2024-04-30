/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Paul Solodrai $
   ======================================================================== */

#include "editor.h"
#include "editor_sort.cpp"
#include "editor_render.cpp"
#include "editor_render_group.cpp"
#include "editor_world.cpp"
#include "editor_asset.cpp"
#include "editor_ui.cpp"

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

#include "editor_text_and_cursors.cpp"
#include "editor_terrain_mode.cpp"
#include "editor_decoration_mode.cpp"
#include "editor_collision_mode.cpp"

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
    Result.Memory = PushSize(Arena, TotalBitmapSize, Align(16, ClearToZero));
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

#if 0
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

    u32 PushBufferSize = Megabytes(1);
    void *PushBuffer = PushSize(GroundMemory.Arena, PushBufferSize);
    void *SortMemory = PushSize(GroundMemory.Arena, PushBufferSize/2);
    void *ClipRectMemory = PushSize(GroundMemory.Arena, PushBufferSize/2);

    game_render_commands Commands = RenderCommandStruct(PushBufferSize, PushBuffer, 
                                                        (u32)Buffer->Width,
                                                        (u32)Buffer->Height);

    render_group RenderGroup = BeginRenderGroup(TranState->Assets, &Commands, TranState->MainGenerationID, true);
    Orthographic(&RenderGroup, Buffer->Width, Buffer->Height, Buffer->Width / Width);
    Clear(&RenderGroup, V4(1.0f, 0.0f, 0.0f, 1.0f));
    object_transform Transform = DefaultFlatTransform();
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
                    Transform.OffsetP = V3(P, 0);
                    PushBitmap(&RenderGroup, Transform, ID, 1.0f, V3(0, 0, 0));
                }
            }
        }
    }

    SortEntries(&Commands, SortMemory);
    LinearizeClipRects(&Commands, ClipRectMemory);
    SoftwareRenderCommands(TranState->HighPriorityQueue, &Commands, Buffer);        
    EndRenderGroup(&RenderGroup);

    // TODO(paul): Findout when to deallocate textures for groundchunks
    if(Buffer->TextureHandle)
    {
        Platform.DeallocateTexture(Buffer->TextureHandle);
        Buffer->TextureHandle = 
            Platform.AllocateTexture(Buffer->Width, Buffer->Height, Buffer->Memory);
    }
    else
    {
        Buffer->TextureHandle = 
            Platform.AllocateTexture(Buffer->Width, Buffer->Height, Buffer->Memory);
    }
    
    EndTemporaryMemory(GroundMemory);
}

static u32 MainTileSurfaces[8][7] = 
{
    {  21,   60,   99,  138,  177,  216,  255},
    { 294,  333,  372,  411,  450,  489,  528},
    { 567,  609,  651,  693,  735,  777,  819},
    { 861,  903,  945,  987, 1029, 1071, 1113},
    {1155, 1194, 1233, 1272, 1311, 1350, 1389},
    {1428, 1456, 1484, 1512, 1540, 1568, 1596},
    {1624, 1663, 1702, 1741, 1780, 1819, 1858},
    {1897, 1936, 1975, 2014, 2053, 2092, 2131}
};

static u32 SubTileSurfaceCounts[8] = 
{
    18, 18, 21, 21, 18, 7, 18, 18, 
};

static u32 SubTileSurfaces[8][21] = 
{
    {  22,   23,   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39},
    { 295,  296,  297,  298,  299,  300,  301,  302,  303,  304,  305,  306,  307,  308,  309,  310,  311,  312},
    { 568,  569,  570,  571,  572,  573,  574,  575,  576,  577,  578,  579,  580,  581,  582,  583,  584,  585,  586,  587,  588},
    { 862,  863,  864,  865,  866,  867,  868,  869,  870,  871,  872,  873,  874,  875,  876,  877,  878,  879,  880,  881,  882},
    {1156, 1157, 1158, 1159, 1160, 1161, 1162, 1163, 1164, 1165, 1166, 1167, 1168, 1169, 1170, 1171, 1172, 1173},
    {1429, 1430, 1431, 1432, 1433, 1434, 1435},
    {1625, 1626, 1627, 1628, 1629, 1630, 1631, 1632, 1633, 1634, 1635, 1636, 1637, 1638, 1639, 1640, 1641, 1642},
    {1898, 1899, 1900, 1901, 1902, 1903, 1904, 1905, 1906, 1907, 1908, 1909, 1910, 1911, 1912, 1913, 1914, 1915}
};

#include <stdlib.h>
#include <time.h>

internal void
InitializeWorldTilesAndDecorations(render_group *RenderGroup, game_assets *Assets, game_state *GameState,
                                   char *TilesFileName, char *DecorationsFileName, char *CollisionsFileName)
{
    time_t t;
    srand((unsigned) time(&t));

    debug_read_file_result WorldTiles = Platform.DEBUGReadEntireFile(TilesFileName);
    if(WorldTiles.Contents)
    {
        GameState->WorldTiles = (world_tile *)WorldTiles.Contents;
        for(u32 TileIndex = 0;
            TileIndex < GameState->WorldTileCount;
            ++TileIndex)
        {
            b32 Found = false;
            world_tile *Tile = GameState->WorldTiles + TileIndex;
            GameState->TileIDs[TileIndex] = Tile->TileID;
            Tile->TileBitmapID = GetBitmapForTile(Assets, GlobalTilesetInfo, GlobalTileset, Tile->TileID);
#if 1
            for(u32 MainIndex = 0;
                MainIndex < 8;
                ++MainIndex)
            {
                for(u32 SubIndex = 0;
                    SubIndex < SubTileSurfaceCounts[MainIndex];
                    ++SubIndex)
                {
                    u32 TestTile = SubTileSurfaces[MainIndex][SubIndex] + GlobalTileset->BitmapIDOffset;
                    if(Tile->TileBitmapID.Value == TestTile)
                    {
                        Tile->TileID = MainTileSurfaces[MainIndex][0] - 1;
                        Tile->TileBitmapID.Value = MainTileSurfaces[MainIndex][0] + GlobalTileset->BitmapIDOffset;
                        Found = true;
                        break;
                    }
                }

                if(Found)
                {
                    break;
                }
            }
#endif

#if 1
            for(u32 MainIndex = 0;
                MainIndex < 8;
                ++MainIndex)
            {
                u32 RandomChance = 1 + rand() % (100 - 1 + 1);

                u32 TestTile = MainTileSurfaces[MainIndex][0] + GlobalTileset->BitmapIDOffset;
                if(Tile->TileBitmapID.Value == TestTile)
                {
                    if(RandomChance > 85)
                    {
                        u32 Min = 1;
                        u32 Max = SubTileSurfaceCounts[MainIndex];
                        u32 RandomIndex = Min + rand() % (Max - Min + 1);

                        Tile->TileID = SubTileSurfaces[MainIndex][RandomIndex - 1] - 1;
                        Tile->TileBitmapID.Value = SubTileSurfaces[MainIndex][RandomIndex - 1] + GlobalTileset->BitmapIDOffset;
                    }

                    break;
                }
            }
#endif
        }
    }
    else
    {
        for(u32 TileIndex = 0;
            TileIndex < GameState->WorldTileCount;
            ++TileIndex)
        {
            world_tile *WorldTile = GameState->WorldTiles + TileIndex;
            GameState->TileIDs[TileIndex] = 20;
            WorldTile->TileID = 20;
            WorldTile->TileBitmapID.Value = 421;
        }
    }

    debug_read_file_result Decorations = Platform.DEBUGReadEntireFile(DecorationsFileName);
    if(Decorations.Contents)
    {
        GameState->Decorations = (decoration *)Decorations.Contents;

        for(u32 DecorationIndex = 0;
            DecorationIndex < GameState->WorldTileCount;
            ++DecorationIndex)
        {
            decoration *Decoration = GameState->Decorations + DecorationIndex;
            asset_vector MatchVector = {};
            asset_vector WeightVector = {};
            for(u32 DecorationTagIndex = 0;
                DecorationTagIndex < Decoration->TagCount;
                ++DecorationTagIndex)
            {
                ssa_tag *Tag = Decoration->Tags + DecorationTagIndex;
                MatchVector.E[Tag->ID] = Tag->Value;
                WeightVector.E[Tag->ID] = 1.0f;
            }

            if(Decoration->IsSpriteSheet)
            {
                Decoration->SpriteSheetID = GetBestMatchSpriteSheetFrom(Assets,
                                                                        Asset_SpriteSheet,
                                                                        &MatchVector,
                                                                        &WeightVector);
            }
            else
            {
                Decoration->BitmapID = GetBestMatchBitmapFrom(Assets,
                                                              (asset_type_id)Decoration->AssetTypeID,
                                                              &MatchVector,
                                                              &WeightVector);
            }

            if(Decoration->Height == 0.0f)
            {
                ZeroStruct(*Decoration);
            }
        }
        
        for(u32 DecorationIndex = 0;
            DecorationIndex < GameState->WorldTileCount;
            ++DecorationIndex)
        {
            decoration *Decoration = GameState->Decorations + DecorationIndex;
            if(Decoration->IsSpriteSheet)
            {
                animated_decoration *AnimatedDecoration = GameState->AnimatedDecorations + DecorationIndex;
                AnimatedDecoration->SpriteSheet = PushSpriteSheet(RenderGroup, Decoration->SpriteSheetID, true);
                AnimatedDecoration->Info = GetSpriteSheetInfo(Assets, Decoration->SpriteSheetID);
                AnimatedDecoration->SpriteIndex = 0;
            }
        }
    }

    debug_read_file_result Collisions = Platform.DEBUGReadEntireFile(CollisionsFileName);
    if(Collisions.Contents)
    {
        GameState->Collisions = (collision *)Collisions.Contents;
    }

    GameState->WorldTilesInitialized = true;
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
ShowTileCursor(game_state *GameState, render_group *RenderGroup, object_transform Transform,
               world_position MouseChunkP, r32 TileSideInMeters, v2 D)
{
    
    Transform.OffsetP = V3(0, 0, 0);

    v2 Delta = Subtract(GameState->World, &GameState->CameraP, &MouseChunkP);
    PushRect(RenderGroup, Transform, V3(-Delta, 0),
             0.2f*V2(TileSideInMeters, TileSideInMeters), V4(0, 0, 1, 1));

    Transform.SortBias = 10.0f;
    PushRectOutline(RenderGroup, Transform, V3(-D, 0), V2(TileSideInMeters, TileSideInMeters),
                    V4(1.0f, 0.0f, 0.0f, 1), 0.02f);
    Transform.SortBias = 0.0f;
    
    PushRect(RenderGroup, Transform, V3(0, 0, 0), 0.2f*V2(TileSideInMeters, TileSideInMeters), V4(1, 0, 0, 1));
}

internal void
UpdateGroundChunks(game_state *GameState, transient_state *TranState, world *World,
                   rectangle2 CameraBoundsInMeters, r32 TileSideInMeters)
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

internal void
RenderGroundChunks(game_state *GameState, transient_state *TranState, world *World,
                   render_group *RenderGroup, object_transform Transform)
{
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
            Transform.SortBias = -1000.0f;
            PushBitmap(RenderGroup, Transform, Bitmap, GroundSideInMeters, V3(Delta, 0));

            PushRectOutline(RenderGroup, Transform, V3(Delta, 0), V2(GroundSideInMeters, GroundSideInMeters),
                            V4(1.0f, 1.0f, 0.0f, 1.0f), 0.03f);
        }
    }
}

internal void
RenderDecorations(game_state *GameState, rectangle2 SimBounds, render_group *RenderGroup,
                  object_transform Transform)
{
    for(u32 DecorationIndex = 0;
        DecorationIndex < GameState->WorldTileCount;
        ++DecorationIndex)
    {
        decoration *Decoration = GameState->Decorations + DecorationIndex;
        
        if(Decoration->IsSpriteSheet)
        {
            animated_decoration *AnimatedDecoration = GameState->AnimatedDecorations + Decoration->DecorationIndex;
            AnimatedDecoration->SpriteIndex = GetSpriteIndex(GameState->Time,
                                                             AnimatedDecoration->Info->SpriteCount);

            bitmap_id BitmapID = AnimatedDecoration->SpriteSheet->SpriteIDs[AnimatedDecoration->SpriteIndex];
            BitmapID.Value += AnimatedDecoration->SpriteSheet->BitmapIDOffset;

            if(IsValid(Decoration->BitmapID))
            {
                v2 Delta = Subtract(GameState->World, &Decoration->P, &GameState->CameraP) - V2(2.0f, 2.0f);
                if(IsInRectangle(SimBounds, Delta))
                {
                    PushBitmap(RenderGroup, Transform, BitmapID, Decoration->Height, V3(Delta, 0));
                }
            }
        }
        else
        {
            if(IsValid(Decoration->BitmapID))
            {
                v2 Delta = Subtract(GameState->World, &Decoration->P, &GameState->CameraP) - V2(2.0f, 2.0f);
                if(IsInRectangle(SimBounds, Delta))
                {
                    PushBitmap(RenderGroup, Transform, Decoration->BitmapID, Decoration->Height, V3(Delta, 0));
                }
            }
        }
    }
}
#endif

internal void
SetEditMode(editor_state *EditorState, transient_state *TranState, edit_mode EditMode)
{
    b32 NeedToWait = false;
    for(u32 TaskIndex = 0;
        TaskIndex < ArrayCount(TranState->Tasks);
        ++TaskIndex)
    {
        NeedToWait = NeedToWait || TranState->Tasks[TaskIndex].DependsOnEditMode;
    }
    if(NeedToWait)
    {
        Platform.CompleteAllWork(TranState->LowPriorityQueue);
    }

    Clear(&EditorState->ModeArena);
    EditorState->EditMode = EditMode;
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

    v2 WorldChunkDimInMeters =
        {
            TILES_PER_CHUNK * TileSideInMeters,
            TILES_PER_CHUNK * TileSideInMeters
        };

    Assert(sizeof(editor_state) <= Memory->PermanentStorageSize);    
    editor_state *EditorState = (editor_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        memory_arena TotalArena;
        InitializeArena(&TotalArena, Memory->PermanentStorageSize - sizeof(editor_state),
                        (uint8 *)Memory->PermanentStorage + sizeof(editor_state));
        SubArena(&EditorState->ModeArena, &TotalArena, GetArenaSizeRemaining(&TotalArena));
#if 0
        uint32 TilesPerWidth = 30;
        uint32 TilesPerHeight = 17;
        InitializeArena(&EditorState->WorldArena, Memory->PermanentStorageSize - sizeof(editor_state),
                        (uint8 *)Memory->PermanentStorage + sizeof(editor_state));


        EditorState->WorldTileCount = WORLD_HEIGHT_TILE_COUNT*WORLD_WIDTH_TILE_COUNT;
        EditorState->WorldTiles = PushArray(&EditorState->WorldArena, EditorState->WorldTileCount, world_tile);
        EditorState->Decorations = PushArray(&EditorState->WorldArena, EditorState->WorldTileCount, decoration);
//        EditorState->Decorations_ = PushArray(&EditorState->WorldArena, EditorState->WorldTileCount, decoration_);
        EditorState->Collisions = PushArray(&EditorState->WorldArena, EditorState->WorldTileCount, collision);
        EditorState->TileIDs = PushArray(&EditorState->WorldArena, EditorState->WorldTileCount, u32);
        EditorState->AnimatedDecorations = PushArray(&EditorState->WorldArena, EditorState->WorldTileCount,
                                                   animated_decoration);

        for(u32 CollisionIndex = 0;
            CollisionIndex < EditorState->WorldTileCount;
            ++CollisionIndex)
        {
            collision *Collision = EditorState->Collisions + CollisionIndex;
            Collision->P = NullPosition();
        }
        
        EditorState->World = PushStruct(&EditorState->WorldArena, world);
        world *World = EditorState->World;
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
        EditorState->CameraP = NewCameraP;

        EditorState->CameraBoundsMin.ChunkX = 0;
        EditorState->CameraBoundsMin.ChunkY = 0;
    
        EditorState->CameraBoundsMax.ChunkX = WORLD_WIDTH_TILE_COUNT / TILES_PER_CHUNK;
        EditorState->CameraBoundsMax.ChunkY = WORLD_HEIGHT_TILE_COUNT / TILES_PER_CHUNK;

        // NOTE(paul): Initialize cursors and sets stats
//        InitializeCursor(&EditorState->TileMenuBarCursor, 10);
//        InitializeCursor(&EditorState->AssetMenuBarCursor, 2);

        InitializeStringArrayCursor(&EditorState->TestCursor, 1, AssetTypes);

        EditorState->TileSetStats = {};
        EditorState->AssetSetStats = {};
        EditorState->AssetSetStats.Type = Asset_Bole;

        EditorState->EditMode = EditMode_Assets;
#endif        
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

        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(256), TranState);
        
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

//        EditorState->GlobalTilesetID = ID;

        TranState->UIContext = PushStruct(&TranState->TranArena, ui_context);
        SubArena(&TranState->UIContext->ContextArena, &TranState->TranArena, Megabytes(2));
        
        TranState->IsInitialized = true;
    }

    if(TranState->MainGenerationID)
    {
        EndGeneration(TranState->Assets, TranState->MainGenerationID);
    }
    TranState->MainGenerationID = BeginGeneration(TranState->Assets);
    
#if 0    
    if(Input->ExecutableReloaded)
    {
        ResetGroundBuffers(TranState, 0);
    }

    world *World = EditorState->World;

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
        
        EditorState->CameraP = MapIntoChunkSpace(World, EditorState->CameraP, NewP);

        if(Controller->ChangeEditMode.EndedDown)
        {
            EditorState->EditMode += 1;
            if(EditorState->EditMode >= EditMode_Count)
            {
                EditorState->EditMode = 0;
            }
        }

        if(WasPressed(Controller->RightShoulder))
        {
            EditorState->AllowEdit = !EditorState->AllowEdit;
        }
        
        if(EditorState->EditMode == EditMode_Terrain)
        {
            if(Controller->Biome.EndedDown)
            {
                EditorState->TileSetStats.Biome += 1;
                if(EditorState->TileSetStats.Biome >= BiomeType_Count)
                {
                    EditorState->TileSetStats.Biome = 0;
                }
            }

            if(Controller->Type.EndedDown)
            {
                EditorState->TileSetStats.Type += 1;
                if(EditorState->TileSetStats.Type >= TileType_Count)
                {
                    EditorState->TileSetStats.Type = 0;
                }
            }

            if(Controller->Height.EndedDown)
            {
                EditorState->TileSetStats.Height += 1;
                if(EditorState->TileSetStats.Height >= Height_Count)
                {
                    EditorState->TileSetStats.Height = 0;
                }
            }

            if(Controller->CliffHillType.EndedDown)
            {
                EditorState->TileSetStats.CliffHillType += 1;
                if(EditorState->TileSetStats.CliffHillType >= CliffHillType_Count)
                {
                    EditorState->TileSetStats.CliffHillType = 0;
                }
            }

            if(Controller->MainSurface.EndedDown)
            {
                EditorState->TileSetStats.MainSurface += 1;
                if(EditorState->TileSetStats.MainSurface >= TileSurface_Count)
                {
                    EditorState->TileSetStats.MainSurface = 0;
                }
            }
            if(Controller->MergeSurface.EndedDown)
            {
                EditorState->TileSetStats.MergeSurface += 1;
                if(EditorState->TileSetStats.MergeSurface >= TileSurface_Count)
                {
                    EditorState->TileSetStats.MergeSurface = 0;
                }
            }
        }
        else if(EditorState->EditMode == EditMode_Decoration)
        {
            if(Controller->Biome.EndedDown)
            {
                EditorState->AssetSetStats.Biome += 1;
                if(EditorState->AssetSetStats.Biome >= BiomeType_Count)
                {
                    EditorState->AssetSetStats.Biome = 0;
                }
            }

            if(Controller->Type.EndedDown)
            {
                EditorState->AssetSetStats.Type += 1;
                if(EditorState->AssetSetStats.Type >= Asset_Count)
                {
                    EditorState->AssetSetStats.Type = 0;
                }
            }
        }

        if(Controller->ActionUp.EndedDown)
        {
//            ChangeChoosenAttributeValueFor(EditorState, true);
        }
        if(Controller->ActionDown.EndedDown)
        {
//            ChangeChoosenAttributeValueFor(EditorState, false);
        }
    }
#endif    

    //
    // NOTE(casey): Render
    //

    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    
    loaded_bitmap DrawBuffer = {};
    DrawBuffer.Width = RenderCommands->Width;
    DrawBuffer.Height = RenderCommands->Height;

    // TODO(casey): Decide what our pushbuffer size is!
    render_group RenderGroup_ = BeginRenderGroup(TranState->Assets, RenderCommands,
                                                 TranState->MainGenerationID, false);
    render_group *RenderGroup = &RenderGroup_;
    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)DrawBuffer.Width*WidthOfMonitor;// / 2.0f;
    real32 FocalLength = 0.5f;
    real32 DistanceAboveTarget = 10.0f;
    Perspective(RenderGroup, DrawBuffer.Width, DrawBuffer.Height, MetersToPixels, FocalLength, DistanceAboveTarget);

#if 0
    // NOTE(paul): Load GlobalTileset
    GlobalTileset = PushTileset(RenderGroup, EditorState->GlobalTilesetID, true);
    GlobalTilesetInfo = GetTilesetInfo(TranState->Assets, EditorState->GlobalTilesetID);
    if(!EditorState->WorldTilesInitialized)
    {
        InitializeWorldTilesAndDecorations(RenderGroup, TranState->Assets, EditorState,
                                           "worldtiles.bin", "decorations.bin", "collisions.bin");
    }
#endif
    
    Clear(RenderGroup, V4(0.301960784314f, 0.188235294118f, 0.125490196078f, 1));
//    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

    v2 ScreenCenter = {0.5f*(real32)DrawBuffer.Width,
                       0.5f*(real32)DrawBuffer.Height};

    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
    rectangle2 CameraBoundsInMeters = RectMinMax(ScreenBounds.Min, ScreenBounds.Max);

    v2 SimBoundsExpansion = {15.0f, 14.0f};
    rectangle2 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);

    object_transform Transform = DefaultUprightTransform();    

    r32 MouseX = (r32)Input->MouseX;
    r32 MouseY = (r32)Input->MouseY;
    v2 P = Unproject(RenderGroup, Transform, V2(MouseX, MouseY)).xy;
#if 0
    world_position MouseChunkP = MapIntoChunkSpace(World, EditorState->CameraP, P);
    tile_position MouseTileP = TilePositionFromChunkPosition(&MouseChunkP);

    tile_position Tp = TilePositionFromChunkPosition(&MouseChunkP);
    tile_position TCp = TilePositionFromChunkPosition(&EditorState->CameraP);

    v2 dTile =
        {
            (real32)TCp.TileX - (real32)Tp.TileX,
            (real32)TCp.TileY - (real32)Tp.TileY
        };

    v2 D = dTile*TileSideInMeters - V2(0.5f, 0.5f);

    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "Edit Mode: %s",
                EditModeText[EditorState->EditMode]);
    DEBUGTextLine(&TextRenderGroup, TextBuffer);
    
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "CameraP in Chunks: X: %d Y: %d, OX: %f, OY: %f",
                EditorState->CameraP.ChunkX, EditorState->CameraP.ChunkY,
                EditorState->CameraP.Offset_.x, EditorState->CameraP.Offset_.y);
    DEBUGTextLine(&TextRenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "MouseP in Chunks: X: %d Y: %d, OX: %f, OY: %f",
                MouseChunkP.ChunkX, MouseChunkP.ChunkY,
                MouseChunkP.Offset_.x, MouseChunkP.Offset_.y);
    DEBUGTextLine(&TextRenderGroup, TextBuffer);

    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "MouseP in Tiles: X: %d Y: %d",
                MouseTileP.TileX, MouseTileP.TileY);
    DEBUGTextLine(&TextRenderGroup, TextBuffer);

    if(EditorState->RenderGround)
    {
        // NOTE(casey): Ground chunk updating
        UpdateGroundChunks(EditorState, TranState, World, CameraBoundsInMeters, TileSideInMeters);

        // NOTE(casey): Ground chunk rendering
        RenderGroundChunks(EditorState, TranState, World, RenderGroup, Transform);
    }

    Transform.SortBias = 0.0f;

    if(EditorState->RenderDecorations)
    {
        // NOTE(paul): Render decorations
        RenderDecorations(EditorState, SimBounds, RenderGroup, Transform);
    }
#endif

    ui_context *UIContext = TranState->UIContext;
    BeginUI(UIContext, RenderCommands, TranState->Assets, TranState->MainGenerationID, DrawBuffer.Width, DrawBuffer.Height);
    
    layout Layout = BeginLayout(UIContext, UIContext->LastMouseP,
                                V2(UIContext->LeftEdge + 10.0f, 0.5f*UIContext->Height - 10.0f));

    ui_item_id CID = {};
    CID.Value = 10;
    CID.ItemOwner = 1;
    CID.ItemIndex = 4;

    ui_item_id TID = {};
    TID.Value = 8;
    TID.ItemOwner = 2;
    TID.ItemIndex = 6;

    ui_item_id DID = {};
    DID.Value = 12;
    DID.ItemOwner = 523;
    DID.ItemIndex = 333;

    ui_item_id WID = {};
    WID.Value = 13;
    WID.ItemOwner = 55;
    WID.ItemIndex = 14;

    ui_view *WView = GetOrCreateDebugViewFor(UIContext, WID);
    if((WView->InlineBlock.Dim.x == 0) && (WView->InlineBlock.Dim.y == 0))
    {
        WView->InlineBlock.Dim.x = 200;
        WView->InlineBlock.Dim.y = 200;
    }

    ui_interaction WindowInteraction = {};

//    DrawWindow(UIContext, &Layout, &WView->InlineBlock.Dim, WindowInteraction);

    
    switch(EditorState->EditMode)
    {
        case EditMode_Assets:
        {
            ui_item_id AID = {};
            AID.Value = 6;
            AID.ItemOwner = 21;
            AID.ItemIndex = 66;

            BeginRow(&Layout);
            ActionButton(&Layout, "Collision", SetUInt32Interaction(CID, (u32 *)&EditorState->EditMode, EditMode_Collision));
            ActionButton(&Layout, "Terrain", SetUInt32Interaction(TID, (u32 *)&EditorState->EditMode, EditMode_Terrain));
            ActionButton(&Layout, "Decoration", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Decoration));
            EndRow(&Layout);

            BeginRow(&Layout);
//            ActionButton(&Layout, "Bitmap", SetUInt32Interaction(CID, (u32 *)&EditorState->AssetAddMode, AssetMode_AddBitmap));
//            ActionButton(&Layout, "Terrain", SetUInt32Interaction(TID, (u32 *)&EditorState->EditMode, EditMode_Terrain));
//            ActionButton(&Layout, "Decoration", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Decoration));
            EndRow(&Layout);
    
//            u32 Result = SimpleScrollElement(&Layout, &EditorState->TestCursor, V2(200.0f, 50.0f),
//                                             AdvanceArrayCursorInteraction(AID, &EditorState->TestCursor, Asset_Count));
#if 0
            switch(EditorState->AssetAddMode)
            {
                case AssetMode_AddBitmap:
                {
                } break;
            }
#endif
        } break;

        case EditMode_Terrain:
        {
            BeginRow(&Layout);
            ActionButton(&Layout, "Collision", SetUInt32Interaction(CID, (u32 *)&EditorState->EditMode, EditMode_Collision));
            ActionButton(&Layout, "Decoration", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Decoration));
            ActionButton(&Layout, "Assets", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Assets));
            EndRow(&Layout);
#if 0

            BeginRow(&Layout);
            BooleanButton(&Layout, "RenderGround", EditorState->RenderGround,
                          SetUInt32Interaction(CID, (u32 *)&EditorState->RenderGround, !EditorState->RenderGround));
            BooleanButton(&Layout, "RenderDecorations", EditorState->RenderDecorations,
                          SetUInt32Interaction(CID, (u32 *)&EditorState->RenderDecorations, !EditorState->RenderDecorations));
            BooleanButton(&Layout, "AllowEdit", EditorState->AllowEdit,
                          SetUInt32Interaction(DID, (u32 *)&EditorState->AllowEdit, !EditorState->AllowEdit));
            BooleanButton(&Layout, "ShowBorders", EditorState->ShowBorders,
                          SetUInt32Interaction(DID, (u32 *)&EditorState->ShowBorders, !EditorState->ShowBorders));
            EndRow(&Layout);

            TerrainEditMode(RenderGroup, EditorState, TranState, Input, &MouseChunkP, TileSideInMeters, &Layout);
            ShowTileCursor(EditorState, RenderGroup, Transform, MouseChunkP, TileSideInMeters, D);
#endif
        } break;

        case EditMode_Decoration:
        {
            BeginRow(&Layout);
            ActionButton(&Layout, "Collision", SetUInt32Interaction(CID, (u32 *)&EditorState->EditMode, EditMode_Collision));
            ActionButton(&Layout, "Terrain", SetUInt32Interaction(TID, (u32 *)&EditorState->EditMode, EditMode_Terrain));
            ActionButton(&Layout, "Assets", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Assets));
            EndRow(&Layout);
#if 0

            BeginRow(&Layout);
            BooleanButton(&Layout, "RenderGround", EditorState->RenderGround,
                          SetUInt32Interaction(CID, (u32 *)&EditorState->RenderGround, !EditorState->RenderGround));
            BooleanButton(&Layout, "RenderDecorations", EditorState->RenderDecorations,
                          SetUInt32Interaction(CID, (u32 *)&EditorState->RenderDecorations, !EditorState->RenderDecorations));
            BooleanButton(&Layout, "AllowEdit", EditorState->AllowEdit,
                          SetUInt32Interaction(DID, (u32 *)&EditorState->AllowEdit, !EditorState->AllowEdit));
            BooleanButton(&Layout, "ShowBorders", EditorState->ShowBorders,
                          SetUInt32Interaction(DID, (u32 *)&EditorState->ShowBorders, !EditorState->ShowBorders));
            EndRow(&Layout);

            DecorationEditMode(RenderGroup, EditorState, TranState, Input, &MouseChunkP,
                               TileSideInMeters, PixelsToMeters, D);
            ShowTileCursor(EditorState, RenderGroup, Transform, MouseChunkP, TileSideInMeters, D);
#endif
        } break;

        case EditMode_Collision:
        {
            BeginRow(&Layout);
            ActionButton(&Layout, "Terrain", SetUInt32Interaction(TID, (u32 *)&EditorState->EditMode, EditMode_Terrain));
            ActionButton(&Layout, "Decoration", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Decoration));
            ActionButton(&Layout, "Assets", SetUInt32Interaction(DID, (u32 *)&EditorState->EditMode, EditMode_Assets));
            EndRow(&Layout);
#if 0

            BeginRow(&Layout);
            BooleanButton(&Layout, "RenderGround", EditorState->RenderGround,
                          SetUInt32Interaction(CID, (u32 *)&EditorState->RenderGround, !EditorState->RenderGround));
            BooleanButton(&Layout, "RenderDecorations", EditorState->RenderDecorations,
                          SetUInt32Interaction(CID, (u32 *)&EditorState->RenderDecorations, !EditorState->RenderDecorations));
            BooleanButton(&Layout, "AllowEdit", EditorState->AllowEdit,
                          SetUInt32Interaction(DID, (u32 *)&EditorState->AllowEdit, !EditorState->AllowEdit));
            BooleanButton(&Layout, "ShowBorders", EditorState->ShowBorders,
                          SetUInt32Interaction(DID, (u32 *)&EditorState->ShowBorders, !EditorState->ShowBorders));
            EndRow(&Layout);

            // NOTE(paul): Render collisions
            for(u32 CollisionIndex = 0;
                CollisionIndex < EditorState->WorldTileCount;
                ++CollisionIndex)
            {
                collision *Collision = EditorState->Collisions + CollisionIndex;
                if(IsValid(Collision->P))
                {
                    v2 Delta = Subtract(EditorState->World, &Collision->P, &EditorState->CameraP) - V2(2.0f, 2.0f);
                    if(IsInRectangle(SimBounds, Delta))
                    {
                        Transform.OffsetP = V3(Delta, 0);
                        PushRectOutline(RenderGroup, Transform, Collision->Rect, 0.0f, V4(0, 0, 1, 1), 0.025f);
                    }
                }
            }

            CollisionEditMode(RenderGroup, EditorState, TranState, Input, &MouseChunkP,
                              TileSideInMeters);
            ShowTileCursor(EditorState, RenderGroup, Transform, MouseChunkP, TileSideInMeters, D);
#endif
        } break;
    }
    
    EndLayout(&Layout);

    Transform.OffsetP = V3(0, 0, 0);
#if 0
    if(EditorState->ShowBorders)
    {
        PushRectOutline(RenderGroup, Transform, V3(0, 0, 0), GetDim(ScreenBounds), V4(1.0f, 1.0f, 0.0f, 1));
        PushRectOutline(RenderGroup, Transform, V3(0, 0, 0), GetDim(SimBounds), V4(1.0f, 0.0f, 1.0f, 1));
    }

    // NOTE(paul): Advance Time
    EditorState->Time += Input->dtForFrame;
#endif
    
    EndUI(UIContext, Input);

    EndRenderGroup(RenderGroup);
    EndTemporaryMemory(RenderMemory);
    
//    CheckArena(&EditorState->WorldArena);
    CheckArena(&TranState->TranArena);
}



