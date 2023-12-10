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

struct add_low_entity_result
{
    low_entity *Low;
    uint32 LowIndex;
};

inline world_position
ChunkPositionFromTilePosition(world *World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ,
                              v3 AdditionalOffset = V3(0, 0, 0))
{
    world_position BasePos = {};

    real32 TileSideInMeters = 1.0f;
    real32 TileDepthInMeters = 3.0f;
    
    v3 TileDim = V3(TileSideInMeters, TileSideInMeters, TileDepthInMeters);
    v3 Offset = Hadamard(TileDim, V3((real32)AbsTileX, (real32)AbsTileY, (real32)AbsTileZ));
    world_position Result = MapIntoChunkSpace(World, BasePos, AdditionalOffset + Offset);
    
    Assert(IsCanonical(World, Result.Offset_));
    
    return(Result);
}

internal add_low_entity_result
AddLowEntity(game_state *GameState, entity_type Type, world_position P)
{
    Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
    uint32 EntityIndex = GameState->LowEntityCount++;
   
    low_entity *EntityLow = GameState->LowEntities + EntityIndex;
    *EntityLow = {};
    EntityLow->Sim.Type = Type;
    EntityLow->P = NullPosition();

    ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, EntityLow, P);

    add_low_entity_result Result;
    Result.Low = EntityLow;
    Result.LowIndex = EntityIndex;
    
    return(Result);
}

internal add_low_entity_result
AddCameraPoint(game_state *GameState)
{
    world_position P = GameState->CameraP;
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Camera, P);

    if(GameState->CameraFollowingEntityIndex == 0)
    {
        GameState->CameraFollowingEntityIndex = Entity.LowIndex;
    }

    return(Entity);
}

inline v3
GetEntityGroundPoint(sim_entity *Entity, v3 ForEntityP)
{
    v3 Result = ForEntityP;

    return(Result);
}

inline v3
GetEntityGroundPoint(sim_entity *Entity)
{
    v3 Result = GetEntityGroundPoint(Entity, Entity->P);

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
    Result.Memory = PushSize(Arena, TotalBitmapSize);
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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{    
    Platform = Memory->PlatformAPI;

    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));

    uint32 GroundBufferWidth = 512; 
    uint32 GroundBufferHeight = 512;

    real32 PixelsToMeters = 1.0f / 64.0f;

    uint32 TileSideInPixels = 64;
    real32 TileSideInMeters = TileSideInPixels * PixelsToMeters;

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        uint32 TilesPerWidth = 30;
        uint32 TilesPerHeight = 17;

        GameState->TypicalFloorHeight = 3.0f;

        v3 WorldChunkDimInMeters = {TILES_PER_CHUNK * TileSideInMeters,
                                    TILES_PER_CHUNK * TileSideInMeters,
                                    GameState->TypicalFloorHeight};

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        
        // NOTE(casey): Reserve entity slot 0 for the null entity
        AddLowEntity(GameState, EntityType_Null, NullPosition());

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        InitializeWorld(World, WorldChunkDimInMeters);

        real32 TileDepthInMeters = GameState->TypicalFloorHeight;
        
        uint32 ScreenBaseX = 0;
        uint32 ScreenBaseY = 0;
        uint32 ScreenBaseZ = 0;
        uint32 ScreenX = ScreenBaseX;
        uint32 ScreenY = ScreenBaseY;
        uint32 AbsTileZ = ScreenBaseZ;
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
        uint32 CameraTileX = ScreenBaseX*TilesPerWidth + TilesPerWidth / 4 + 3;
        uint32 CameraTileY = ScreenBaseY*TilesPerHeight + TilesPerHeight / 4 + 3;
        uint32 CameraTileZ = ScreenBaseZ;
        NewCameraP = ChunkPositionFromTilePosition(GameState->World,
                                                   CameraTileX,
                                                   CameraTileY,
                                                   CameraTileZ);
        GameState->CameraP = NewCameraP;

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
        
        TranState->IsInitialized = true;
    }
    
    
#if 0
    if(Input->ExecutableReloaded)
    {
        ResetGroundBuffers(TranState);
    }
#endif    

    world *World = GameState->World;

    //
    // NOTE(casey): 
    //

    for(int ControllerIndex = 0;
        ControllerIndex < 1;//ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        controlled_camera *ConCamera = GameState->ControlledHeroes + ControllerIndex;
        if(ConCamera->CameraIndex == 0)
        {
            if(Controller->Start.EndedDown)
            {
                *ConCamera = {};
                ConCamera->CameraIndex = AddCameraPoint(GameState).LowIndex;
            }
        }
        else
        {
            ConCamera->ddP = {};
            
            if(Controller->IsAnalog)
            {
                // NOTE(casey): Use analog movement tuning
                ConCamera->ddP = V2(Controller->StickAverageX, Controller->StickAverageY);
            }
            else
            {
                // NOTE(casey): Use digital movement tuning
                if(Controller->MoveUp.EndedDown)
                {
                    ConCamera->ddP.y = 1.0f;
                }
                if(Controller->MoveDown.EndedDown)
                {
                    ConCamera->ddP.y = -1.0f;
                }
                if(Controller->MoveLeft.EndedDown)
                {
                    ConCamera->ddP.x = -1.0f;
                }
                if(Controller->MoveRight.EndedDown)
                {
                    ConCamera->ddP.x = 1.0f;
                }
            }
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

    // TODO(casey): Decide what our pushbuffer size is!
    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false);
    BeginRender(RenderGroup);
    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;
    Prespective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, 0.6f, 9.0f);

    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

    v2 ScreenCenter = {0.5f*(real32)DrawBuffer->Width,
                       0.5f*(real32)DrawBuffer->Height};

    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
    rectangle3 CameraBoundsInMeters = RectMinMax(V3(ScreenBounds.Min, 0.0f),
                                                 V3(ScreenBounds.Max, 0.0f));
    CameraBoundsInMeters.Min.z = -3.0f*GameState->TypicalFloorHeight;
    CameraBoundsInMeters.Max.z =  1.0f*GameState->TypicalFloorHeight;

    // NOTE(casey): Ground chunk rendering
    for(uint32 GroundBufferIndex = 0;
        GroundBufferIndex < TranState->GroundBufferCount;
        ++GroundBufferIndex)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
        if(IsValid(GroundBuffer->P))
        {
            loaded_bitmap *Bitmap = &GroundBuffer->Bitmap;
            v3 Delta = Subtract(GameState->World, &GroundBuffer->P, &GameState->CameraP);        
            if((Delta.z >= -1.0f) && (Delta.z < 1.0f))
            {
                real32 GroundSideInMeters = World->ChunkDimInMeters.x;
                PushBitmap(RenderGroup, Bitmap, GroundSideInMeters, Delta);

//                PushRectOutline(RenderGroup, Delta, V2(GroundSideInMeters, GroundSideInMeters),
//                                V4(1.0f, 1.0f, 0.0f, 1.0f));
            }
        }
    }
    // NOTE(casey): Ground chunk updating
    {
        world_position MinChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMeters));
        world_position MaxChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMeters));

        for(int32 ChunkZ = MinChunkP.ChunkZ;
            ChunkZ <= MaxChunkP.ChunkZ;
            ++ChunkZ)
        {
            for(int32 ChunkY = MinChunkP.ChunkY;
                ChunkY <= MaxChunkP.ChunkY;
                ++ChunkY)
            {
                for(int32 ChunkX = MinChunkP.ChunkX;
                    ChunkX <= MaxChunkP.ChunkX;
                    ++ChunkX)
                {
                    world_position ChunkCenterP = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
                    v3 RelP = Subtract(World, &ChunkCenterP, &GameState->CameraP);

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
                            real32 BufferLengthSq = LengthSq(RelP.xy);
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
//                        FillGroundChunk(TranState, GameState, FurthestBuffer, &ChunkCenterP,
//                                        &GameState->MapBitmap.Bitmap, TileSideInMeters);
                    }
                }
            }
        }
    }

    // TODO(paul): Maybe make sim region for a whole map??
    v3 SimBoundsExpansion = {15.0f, 15.0f, 0.0f};
    rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);
    temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
    world_position SimCenterP = GameState->CameraP;
    sim_region *SimRegion = BeginSim(&TranState->TranArena, GameState, GameState->World,
                                     SimCenterP, SimBounds, Input->dtForFrame);
    v3 CameraP = Subtract(World, &GameState->CameraP, &SimCenterP);
    
    PushRectOutline(RenderGroup, V3(0, 0, 0), GetDim(ScreenBounds), V4(1.0f, 1.0f, 0.0f, 1));
//    PushRectOutline(RenderGroup, V3(0, 0, 0), GetDim(CameraBoundsInMeters).xy, V4(1.0f, 1.0f, 1.0f, 1));
    PushRectOutline(RenderGroup, V3(0, 0, 0), GetDim(SimBounds).xy, V4(0.0f, 1.0f, 1.0f, 1));
    PushRectOutline(RenderGroup, V3(0, 0, 0), GetDim(SimRegion->Bounds).xy, V4(1.0f, 0.0f, 0.0f, 1));

    for(uint32 EntityIndex = 0;
        EntityIndex < SimRegion->EntityCount;
        ++EntityIndex)
    {
        sim_entity *Entity = SimRegion->Entities + EntityIndex;
        if(Entity->Updatable)
        {
            real32 dt = Input->dtForFrame;
        
            // TODO(casey): This is incorrect, should be computed after update!!!!

            v3 ddP = {};

            v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;
            real32 FadeTopEndZ = 0.75f*GameState->TypicalFloorHeight;
            real32 FadeTopStartZ = 0.5f*GameState->TypicalFloorHeight;
            real32 FadeBottomStartZ = -2.0f*GameState->TypicalFloorHeight;
            real32 FadeBottomEndZ = -2.25f*GameState->TypicalFloorHeight;
            RenderGroup->GlobalAlpha = 1.0f;
            if(CameraRelativeGroundP.z > FadeTopStartZ)
            {
                RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeTopEndZ, CameraRelativeGroundP.z, FadeTopStartZ);
            }
            else if(CameraRelativeGroundP.z < FadeBottomStartZ)
            {
                RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeBottomEndZ, CameraRelativeGroundP.z, FadeBottomStartZ);
            }

            //
            // NOTE(casey):
            //

            switch(Entity->Type)
            {
                case EntityType_Camera:
                {
                    for(uint32 ControlIndex = 0;
                        ControlIndex < ArrayCount(GameState->ControlledHeroes);
                        ++ControlIndex)
                    {
                        controlled_camera *ConCamera = GameState->ControlledHeroes + ControlIndex;

                        if(Entity->StorageIndex == ConCamera->CameraIndex)
                        {
                            MoveEntity(Entity, Input->dtForFrame, V3(ConCamera->ddP, 0.0f));
                            PushRect(RenderGroup, Entity->P, V2(0.1f, 0.1f), V4(1, 0, 0, 1));
                        }
                    }
                } break;
            }

            RenderGroup->Transform.OffsetP = GetEntityGroundPoint(Entity);
        }
    }

    asset_vector MatchVector = {};
    MatchVector.E[Tag_TileBiomeType] = BiomeType_AncientForest;
    MatchVector.E[Tag_TileState] = TileState_Solid;
    MatchVector.E[Tag_TileSurface] = TileSurface_3;

    asset_vector WeightVector = {};
    WeightVector.E[Tag_TileBiomeType] = 1.0f;
    WeightVector.E[Tag_TileState] = 1.0f;
    WeightVector.E[Tag_TileSurface] = 0.5f;

    bitmap_id TileBitmapID = GetBestMatchBitmapFrom(TranState->Assets, Asset_Tile, &MatchVector, &WeightVector);

    PushBitmap(RenderGroup, TileBitmapID, 1.0f, V3(0, 0, 0));
    
    RenderGroup->GlobalAlpha = 1.0f;

    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer);
    EndRender(RenderGroup);
    
    EndSim(SimRegion, GameState, CameraBoundsInMeters);
    EndTemporaryMemory(SimMemory);
    EndTemporaryMemory(RenderMemory);
    
    CheckArena(&GameState->WorldArena);
    CheckArena(&TranState->TranArena);
}



