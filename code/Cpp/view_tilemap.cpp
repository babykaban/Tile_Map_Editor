/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "view_tilemap.h"
#include "view_tilemap_render_group.cpp"
#include "view_tilemap_world.cpp"
#include "view_tilemap_sim_region.cpp"

#if 1
// TODO(paul): Learn how to do asset streaming and remove this
#include <stdio.h>
#endif

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 SizeOfBitmap;
    int32 HorzResolution;
    int32 VertResolution;
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};
#pragma pack(pop)

inline v2
TopDownAlign(loaded_bitmap *Bitmap, v2 Align)
{
    Align.y = (real32)(Bitmap->Height - 1) - Align.y;

    Align.x = SafeRatio0(Align.x, (real32)Bitmap->Width);
    Align.y = SafeRatio0(Align.y, (real32)Bitmap->Height);
    
    return(Align);
}

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName,
             int32 AlignX, int32 TopDownAlignY)
{
    loaded_bitmap Result = {};
    
    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);    
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.AlignPercentage = TopDownAlign(&Result, V2i(AlignX, TopDownAlignY));
        Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);
        
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

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName)
{
    loaded_bitmap Result = DEBUGLoadBMP(Thread, ReadEntireFile, FileName, 0, 0);
    Result.AlignPercentage = V2(0.5f, 0.5f);

    return(Result);
}

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

    real32 TileSideInMeters = 1.4f;
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

internal void
FillGroundChunks(transient_state *TranState, game_state *GameState)
{
    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);
#if 0
    for()
    {
    }
        
    loaded_bitmap *Buffer = &GroundBuffer->Bitmap;
    Buffer->AlignPercentage = V2(0.5f, 0.5f);
    Buffer->WidthOverHeight = 1.0f; 
    real32 Width = GameState->World->ChunkDimInMeters.x;
    real32 Height = GameState->World->ChunkDimInMeters.y;
    Assert(Width == Height);
    v2 HalfDim = 0.5f*V2(Width, Height);

    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4));
    Ortographic(RenderGroup, Buffer->Width, Buffer->Height, Buffer->Width / Width);
    Clear(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f));

    TiledRenderGroupToOutput(TranState->RenderQueue, RenderGroup, Buffer);
#endif
    EndTemporaryMemory(GroundMemory);
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
    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
    int32 TotalBitmapSize = Width*Height*BITMAP_BYTES_PER_PIXEL;
    Result.Memory = PushSize(Arena, TotalBitmapSize);
    if(ClearToZero)
    {
        ClearBitmap(&Result);
    }

    return(Result);
}

#if VIEW_TILEMAP_INTERNAL
game_memory *DebugGlobalMemory;
#endif

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

static uint32
LoadTileDataAndIdentities(memory_arena *Arena, loaded_tile *Tiles, loaded_bitmap *TileSheet, int32 TileDim)
{
    Assert(TileSheet->Width == TileSheet->Height);

    uint32 TileCountX = (TileSheet->Width / TileDim);
    uint32 TileCountY = (TileSheet->Height / TileDim);

    uint32 LoadedTileCount = 0; 
    for(uint32 TileY = 0;
        TileY < TileCountY;
        ++TileY)
    {
    
        for(uint32 TileX = 0;
            TileX < TileCountX;
            ++TileX)
        {
            loaded_tile *Tile = Tiles + LoadedTileCount;
            Tile->Bitmap = MakeEmptyBitmap(Arena, TileDim, TileDim);
            loaded_bitmap *Bitmap = &Tile->Bitmap;
            ++LoadedTileCount;

            int32 MinX = TileX * TileDim;
            int32 MinY = TileY * TileDim;
            int32 MaxX = MinX + TileDim;
            int32 MaxY = MinY + TileDim;

            uint8 *SourceRow = ((uint8 *)TileSheet->Memory + MinX*BITMAP_BYTES_PER_PIXEL + MinY*TileSheet->Pitch);
    
            uint8 *DestRow = (uint8 *)Bitmap->Memory;
            uint64 ColorSum = 0;
            for(int Y = MinY;
                Y < MaxY;
                ++Y)
            {
                uint32 *Dest = (uint32 *)DestRow;
                uint32 *Source = (uint32 *)SourceRow;
                for(int X = MinX;
                    X < MaxX;
                    ++X)
                {
                    *Dest = *Source;

                    ColorSum += (uint64)*Dest;
                    ++Dest;
                    ++Source;
                }

                DestRow += Bitmap->Pitch;
                SourceRow += TileSheet->Pitch;
            }

            Tile->Identity = (uint32)(ColorSum / (TileDim*TileDim));
        }
    }

    return(LoadedTileCount);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{    
    PlatformAddEntry = Memory->PlatformAddEntry;
    PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;

#if VIEW_TILEMAP_INTERNAL
    DebugGlobalMemory = Memory;
#endif

    BEGIN_TIMED_BLOCK(GameUpdateAndRender)
                        
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));

    uint32 GroundBufferWidth = 256; 
    uint32 GroundBufferHeight = 256;

    real32 PixelsToMeters = 1.0f / 42.0f;

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        uint32 TilesPerWidth = 33;
        uint32 TilesPerHeight = 19;

        GameState->TypicalFloorHeight = 3.0f;

        v3 WorldChunkDimInMeters = {PixelsToMeters*(real32)GroundBufferWidth,
                                    PixelsToMeters*(real32)GroundBufferHeight,
                                    GameState->TypicalFloorHeight};

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        
        // NOTE(casey): Reserve entity slot 0 for the null entity
        AddLowEntity(GameState, EntityType_Null, NullPosition());

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        InitializeWorld(World, WorldChunkDimInMeters);

        real32 TileSideInMeters = 1.4f;
        real32 TileDepthInMeters = GameState->TypicalFloorHeight;

        GameState->Border = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "tiles\border.bmp");
        GameState->Source = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "tiles\\structured_art.bmp");

        uint32 GroundBufferWidth = 256; 
        uint32 GroundBufferHeight = 256;
        
        GameState->GroundBufferCount = 128;
        GameState->GroundBuffers = PushArray(&GameState->WorldArena, GameState->GroundBufferCount, ground_buffer);
        for(uint32 GroundBufferIndex = 0;
            GroundBufferIndex < GameState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = GameState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->Bitmap = MakeEmptyBitmap(&GameState->WorldArena, GroundBufferWidth, GroundBufferHeight, false);
            GroundBuffer->P = NullPosition();
        }

        loaded_bitmap TileSheet = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "tile_sheet.bmp");
        GameState->LoadedTileCount = LoadTileDataAndIdentities(&GameState->WorldArena, GameState->Tiles,
                                                               &TileSheet, 60);
        
        
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
        uint32 CameraTileX = ScreenBaseX*TilesPerWidth + TilesPerWidth/2;
        uint32 CameraTileY = ScreenBaseY*TilesPerHeight + TilesPerHeight/2;
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

        TranState->RenderQueue = Memory->HighPriorityQueue;
        
        TranState->IsInitialized = true;
    }

    
    
#if 0
    if(Input->ExecutableReloaded)
    {
    FillGroundChunks();
        for(uint32 GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->P = NullPosition();            
        }        
    }
#endif    

    world *World = GameState->World;

    //
    // NOTE(casey): 
    //
    for(int ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
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
    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4));
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

#if 1

    // TODO(paul): Update only updatable ground chunks
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
                    world_position ChunkP = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
                    v3 Delta = Subtract(GameState->World, &ChunkP, &GameState->CameraP);        
                    real32 GroundSideInMeters = World->ChunkDimInMeters.x;
                    PushRectOutline(RenderGroup, Delta, V2(GroundSideInMeters, GroundSideInMeters),
                                    V4(1.0f, 1.0f, 0.0f, 1.0f));
                }
            }
        }
    }
#endif    

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
                            MoveEntity(GameState, SimRegion, Entity, Input->dtForFrame, V3(ConCamera->ddP, 0.0f));
                        }
                    }
                } break;
            }

            RenderGroup->Transform.OffsetP = GetEntityGroundPoint(Entity);
        }
    }

    RenderGroup->GlobalAlpha = 1.0f;
    
    TiledRenderGroupToOutput(TranState->RenderQueue, RenderGroup, DrawBuffer);

    EndSim(SimRegion, GameState);
    EndTemporaryMemory(SimMemory);
    EndTemporaryMemory(RenderMemory);
    
    CheckArena(&GameState->WorldArena);
    CheckArena(&TranState->TranArena);

    END_TIMED_BLOCK(GameUpdateAndRender)
}
