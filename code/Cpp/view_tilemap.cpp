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

internal loaded_bitmap *
FindTileBitmapByIdentity(loaded_tile *Tiles, uint32 TileCount, uint32 Identity)
{
    loaded_bitmap *Result = 0;

    for(uint32 TileIndex = 0;
        TileIndex < TileCount;
        ++TileIndex)
    {
        loaded_tile *Tile = Tiles + TileIndex;
        if(Tile->Identity == Identity)
        {
            Result = &Tile->Bitmap;
            break;
        }
    }
    
    return(Result);
}


internal void
FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP,
                loaded_bitmap *MapBitmap, real32 TileSideInMeters)
{
    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);

    loaded_bitmap *Buffer = &GroundBuffer->Bitmap;
    Buffer->AlignPercentage = V2(0.5f, 0.5f);
    Buffer->WidthOverHeight = 1.0f; 

    real32 Width = GameState->World->ChunkDimInMeters.x;
    real32 Height = GameState->World->ChunkDimInMeters.y;
    Assert(Width == Height);
    v2 HalfDim = 0.5f*V2(Width, Height);

    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4));
    Ortographic(RenderGroup, Buffer->Width, Buffer->Height, Buffer->Width / Width);
    Clear(RenderGroup, V4(1.0f, 0.0f, 1.0f, 1.0f));

    GroundBuffer->P = *ChunkP;

    int32 TileCountX = MapBitmap->Width;
    int32 TileCountY = MapBitmap->Height;

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
                    uint32 *Identity = (uint32 *)MapBitmap->Memory + TileY*TileCountX + TileX;

                    loaded_bitmap *Bitmap = FindTileBitmapByIdentity(GameState->Tiles,
                                                                     GameState->LoadedTileCount, *Identity);
                    real32 X = (real32)(TileX - MinTileX);
                    real32 Y = (real32)(TileY - MinTileY);
                    v2 P = -HalfDim + V2(TileSideInMeters*X, TileSideInMeters*Y);
                    
                    if(Bitmap)
                    {
                        PushBitmap(RenderGroup, Bitmap, TileSideInMeters, V3(P, 0.0f));
                    }
                }
            }
        }
    }

    TiledRenderGroupToOutput(TranState->RenderQueue, RenderGroup, Buffer);
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

internal uint32
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
            Bitmap->WidthOverHeight = SafeRatio0((real32)Bitmap->Width, (real32)Bitmap->Height);
            Bitmap->AlignPercentage = V2(0.0f, 0.0f);
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

internal void
DrawTileIdentity(render_group *RenderGroup, game_state *GameState, transient_state *TranState, v3 Offset)
{
    loaded_bitmap *Map = &GameState->MapBitmap;

    world_position ChunkP = {};
    for(uint32 GroundBufferIndex = 0;
        GroundBufferIndex < TranState->GroundBufferCount;
        ++GroundBufferIndex)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
        if(AreInSameChunk(GameState->World, &GameState->CameraP, &GroundBuffer->P))
        {
            ChunkP = GameState->CameraP;
            break;
        }
    }

    int32 TileCountX = Map->Width;
    int32 TileCountY = Map->Height;

    real32 TileOffsetX = ChunkP.Offset_.x;
    real32 TileOffsetY = ChunkP.Offset_.y;
    if((ChunkP.ChunkX >= 0) && (ChunkP.ChunkY >= 0))
    {
        int32 TileX = TILES_PER_CHUNK*ChunkP.ChunkX;
        int32 TileY = TILES_PER_CHUNK*ChunkP.ChunkY;

        // TODO(paul): Find more efficient way of doing this
        if((TileOffsetX > 0.0f) && (TileOffsetY > 0.0f))
        {
            if(TileOffsetX > 1.0f)
            {
                TileX += 3;
            }
            else
            {
                TileX += 2;
            }

            if(TileOffsetY > 1.0f)
            {
                TileY += 3;
            }
            else
            {
                TileY += 2;
            }
        }
        else if((TileOffsetX > 0.0f) && (TileOffsetY < 0.0f))
        {
            if(TileOffsetX > 1.0f)
            {
                TileX += 3;
            }
            else
            {
                TileX += 2;
            }

            if(TileOffsetY < -1.0f)
            {
                TileY += 0;
            }
            else
            {
                TileY += 1;
            }
        }
        else if((TileOffsetX < 0) && (TileOffsetY > 0))
        {
            if(TileOffsetX < -1.0f)
            {
                TileX += 0;
            }
            else
            {
                TileX += 1;
            }

            if(TileOffsetY > 1.0f)
            {
                TileY += 3;
            }
            else
            {
                TileY += 2;
            }
        }
        else
        {
            if(TileOffsetX < -1.0f)
            {
                TileX += 0;
            }
            else
            {
                TileX += 1;
            }

            if(TileOffsetY < -1.0f)
            {
                TileY += 0;
            }
            else
            {
                TileY += 1;
            }
        }

        GameState->TileIndexInMap = TileY*TileCountX + TileX;
        uint32 *Identity = (uint32 *)Map->Memory + GameState->TileIndexInMap;
        
        v4 Color = (1.0f / 255.0f)*V4((real32)(((*Identity >> 16) & 0xFF)),
                                      (real32)(((*Identity >> 8) & 0xFF)),
                                      (real32)(((*Identity >> 0) & 0xFF)),
                                      (real32)(((*Identity >> 24) & 0xFF)));
        
        PushRect(RenderGroup, Offset, V2(1.0f, 1.0f), Color);
    }
}

internal void
ShowTileMenu(render_group *RenderGroup, game_state *GameState, v2 WindowDim)
{
    v2 HalfWindowDim = 0.5f*WindowDim;
    PushRectOutline(RenderGroup, V3(0.0f, 0.0f, 0.0f), WindowDim, V4(1, 0, 0, 1));
    PushRect(RenderGroup, V3(0.0f, 0.0f, 0.0f), WindowDim, V4(0, 0, 1, 1));

    real32 TileDim = 1.0f;
    real32 TileOffsetX = 0.0f;
    real32 TileOffsetY = 0.0f;
    for(int32 TileIndex = 0;
        TileIndex < GameState->LoadedTileCount;
        ++TileIndex)
    {
        v2 TileOffset = -HalfWindowDim + V2(TileOffsetX, TileOffsetY);
        v3 CursorOffset = V3(TileOffset, 0.0f) + 0.5f*V3(TileDim, TileDim, 0.0f);
        loaded_tile *Tile = GameState->Tiles + TileIndex;
        PushBitmap(RenderGroup, &Tile->Bitmap, TileDim, V3(TileOffset, 0.0f));

        if(GameState->Cursor.TileIndex == TileIndex)
        {
            PushRectOutline(RenderGroup, CursorOffset, V2(TileDim, TileDim),
                            V4(1, 1, 1, 1), 0.015f);
        }

        if((TileOffsetX) > WindowDim.x)
        {
            TileOffsetY += TileDim;
            TileOffsetX = 0.0f;
        }
        else
        {
            TileOffsetX += TileDim;
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

internal void
ChangeGroundTileTexture(game_state *GameState)
{
//    loaded_bitmap *Map = GameState->
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

    real32 PixelsToMeters = 1.0f / 64.0f;

    uint32 TileSideInPixels = 64;
    real32 TileSideInMeters = TileSideInPixels * PixelsToMeters;

    int32 MapWidthInTiles = 15 * 30;
    int32 MapHeightInTiles = 15 * 17;

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

        GameState->Border = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "grass.bmp");
        GameState->Source = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "tiles\\structured_art.bmp");
        GameState->InValidTile = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "invalid_tile.bmp");
        
        loaded_bitmap TileSheet = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "tile_sheet.bmp");
        GameState->LoadedTileCount = LoadTileDataAndIdentities(&GameState->WorldArena, GameState->Tiles,
                                                               &TileSheet, TileSideInPixels);
            
//        GameState->MapBitmap = MakeEmptyBitmap(&GameState->WorldArena, MapWidthInTiles, MapHeightInTiles, true);
        GameState->MapBitmap = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "map_bitmap_3.bmp");
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
        GameState->TileChangingProcess = false;

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
            GameState->Cursor.Direction = CursorDirection_Null;
            
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

                // NOTE(casey): Use digital movement tuning
                if(Controller->ActionUp.EndedDown)
                {
                    GameState->Cursor.Direction = CursorDirection_Up;
                }
                if(Controller->ActionDown.EndedDown)
                {
                    GameState->Cursor.Direction = CursorDirection_Down;
                }
                if(Controller->ActionLeft.EndedDown)
                {
                    GameState->Cursor.Direction = CursorDirection_Left;
                }
                if(Controller->ActionRight.EndedDown)
                {
                    GameState->Cursor.Direction = CursorDirection_Right;
                }

                if(Controller->OpenTileMenu.EndedDown)
                {
                    GameState->TileChangingProcess = true;
                }

                if(Controller->Back.EndedDown)
                {
                    GameState->TileChangingProcess = false;
                }

                if(GameState->TileChangingProcess)
                {
                    if((Controller->ChangeTile.EndedDown) &&
                       !(GameState->ChangeTile))
                    {
                        GameState->ChangeTile = true;
                    }
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

                PushRectOutline(RenderGroup, Delta, V2(GroundSideInMeters, GroundSideInMeters),
                                V4(1.0f, 1.0f, 0.0f, 1.0f));
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
                ChunkY <= MaxChunkP.ChunkY + 1;
                ++ChunkY)
            {
                for(int32 ChunkX = MinChunkP.ChunkX;
                    ChunkX <= MaxChunkP.ChunkX + 1;
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
                            v3 RelP = Subtract(World, &GroundBuffer->P, &GameState->CameraP);
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
                        FillGroundChunk(TranState, GameState, FurthestBuffer, &ChunkCenterP,
                                        &GameState->MapBitmap, TileSideInMeters);
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

    if(GameState->TileChangingProcess)
    {
        v2 WindowDim = V2(20.0f, 12.0f);
        int32 TilesToMoveUpOrDown = (int32)WindowDim.x;
        
        switch(GameState->Cursor.Direction)
        {
            case CursorDirection_Null:
            {
            } break;

            case CursorDirection_Up:
            {
                GameState->Cursor.TileIndex += TilesToMoveUpOrDown;
            } break;

            case CursorDirection_Down:
            {
                GameState->Cursor.TileIndex -= TilesToMoveUpOrDown;
            } break;

            case CursorDirection_Left:
            {
                GameState->Cursor.TileIndex -= 1;
            } break;

            case CursorDirection_Right:
            {
                GameState->Cursor.TileIndex += 1;
            } break;
        }

        if(GameState->Cursor.TileIndex < 0)
        {
            GameState->Cursor.TileIndex = 0;
        }
        if(GameState->Cursor.TileIndex > GameState->LoadedTileCount)
        {
            GameState->Cursor.TileIndex = GameState->LoadedTileCount - 1;
        }
        
        ShowTileMenu(RenderGroup, GameState, WindowDim);

        if(GameState->ChangeTile)
        {
            ChangeGroundTileTexture(GameState);
            ResetGroundBuffers(TranState);
            GameState->ChangeTile = false;
            GameState->TileChangingProcess = false;
        }
    }
    else
    {
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
                                PushRect(RenderGroup, V3(0, 0, 0), V2(0.1f, 0.1f), V4(1, 0, 0, 1));
                            }
                        }
                    } break;
                }

                RenderGroup->Transform.OffsetP = GetEntityGroundPoint(Entity);
            }
        }
    }

    
    RenderGroup->GlobalAlpha = 1.0f;
    
    DrawTileIdentity(RenderGroup, GameState, TranState, V3(-11.0f, 6.0f, 0.0f));

    TiledRenderGroupToOutput(TranState->RenderQueue, RenderGroup, DrawBuffer);

    
    EndSim(SimRegion, GameState);
    EndTemporaryMemory(SimMemory);
    EndTemporaryMemory(RenderMemory);
    
    CheckArena(&GameState->WorldArena);
    CheckArena(&TranState->TranArena);

    END_TIMED_BLOCK(GameUpdateAndRender)
}
