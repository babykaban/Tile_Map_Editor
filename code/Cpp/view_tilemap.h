#if !defined(VIEW_TILEMAP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "view_tilemap_platform.h"

//#include "my_profiler.cpp"

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

//
//
//

struct memory_arena
{
    memory_index Size;
    uint8 *Base;
    memory_index Used;

    int32 TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};

inline void
InitializeArena(memory_arena *Arena, memory_index Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (uint8 *)Base;
    Arena->Used = 0;
    Arena->TempCount = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)

inline void *
PushSize_(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    
    return(Result);
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;

    Result.Arena = Arena;
    Result.Used = Arena->Used;

    ++Arena->TempCount;
    
    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    Assert(Arena->Used >= TempMem.Used);
    Arena->Used = TempMem.Used;
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    // TODO(casey): Check this guy for performance
    uint8 *Byte = (uint8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

#include "view_tilemap_intrinsics.h"
#include "view_tilemap_math.h"
#include "view_tilemap_world.h"
#include "view_tilemap_sim_region.h"
#include "view_tilemap_render_group.h"

struct low_entity
{
    world_position P;
    sim_entity Sim;
};

struct controlled_camera
{
    uint32 CameraIndex;
    // NOTE(casey): These are the controller requests for simulation
    v2 ddP;
};

struct ground_buffer
{
    // NOTE(casey): An invalid P tells us that this ground_buffer has not been filled 
    world_position P; // NOTE(casey): This is the center of the bitmap
    loaded_bitmap Bitmap;
};

struct loaded_tile
{
    uint32 Identity;
    loaded_bitmap Bitmap;
};

enum change_direction
{
    CursorDirection_Null,

    CursorDirection_Up,
    CursorDirection_Down,
    CursorDirection_Left,
    CursorDirection_Right,
};

struct change_cursor
{
    change_direction Direction;
    int32 TileIndex;
};

struct game_state
{
    memory_arena WorldArena;
    world *World;

    // NOTE(paul): Loading staff
    loaded_bitmap Border;
    loaded_bitmap Source;
    loaded_bitmap MapBitmap;
    loaded_bitmap InValidTile;
    
    change_cursor Cursor;

    real32 TypicalFloorHeight;

    uint32 CameraFollowingEntityIndex;
    world_position CameraP;
    uint32 TileIndexInMap;

    controlled_camera ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];
    
    int32 LoadedTileCount;
    loaded_tile Tiles[256];

    uint32 LowEntityCount;
    low_entity LowEntities[10];

    bool32 TileChangingProcess;
    bool32 ChangeTile;
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena TranArena;
    
    uint32 GroundBufferCount;
    ground_buffer *GroundBuffers;

    platform_work_queue *RenderQueue;
};

inline low_entity * 
GetLowEntity(game_state *GameState, uint32 Index)
{
    low_entity *Result = 0;

    if((Index > 0) && (Index < GameState->LowEntityCount))
    {
        Result = GameState->LowEntities + Index;
    }

    return(Result);
}

global_variable platform_add_entry *PlatformAddEntry;
global_variable platform_complete_all_work *PlatformCompleteAllWork;

#define VIEW_TILEMAP_H
#endif
