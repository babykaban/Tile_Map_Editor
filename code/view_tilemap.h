#if !defined(VIEW_TILEMAP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Paul Solodrai $
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

inline memory_index
GetAlignmentOffset(memory_arena *Arena, memory_index Alignment)
{
    memory_index AlignmentOffset = 0;
    
    memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
    memory_index AlignmentMask = Alignment - 1;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
    }

    return(AlignmentOffset);
}

inline memory_index
GetArenaSizeRemaining(memory_arena *Arena, memory_index Alignment = 4)
{
    memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment));

    return(Result);
}

#define PushStruct(Arena, type, ...) (type *)PushSize_(Arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(Arena, Count, type, ...) (type *)PushSize_(Arena, (Count)*sizeof(type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)

inline void *
PushSize_(memory_arena *Arena, memory_index Size, memory_index Alignment = 4)
{
    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Size += AlignmentOffset;

    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;
    
    return(Result);
}

// NOTE(casey): This is generally not for production use, this is probably
// really something we need during testing, but who knows
inline char *
PushString(memory_arena *Arena, char *Source)
{
    u32 Size = 1;
    for(char *At = Source;
        *At;
        ++At)
    {
        ++Size;
    }

    char *Dest = (char *)PushSize_(Arena, Size);
    for(u32 CharIndex = 0;
        CharIndex < Size;
        ++CharIndex)
    {
        Dest[CharIndex] = Source[CharIndex];
    }

    return(Dest);
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

inline void
SubArena(memory_arena *Result, memory_arena *Arena, memory_index Size, memory_index Alignment = 16)
{
    Result->Size = Size;
    Result->Base = (uint8 *)PushSize_(Arena, Size, Alignment);
    Result->Used = 0;
    Result->TempCount = 0;
}

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
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

inline void
Copy(memory_index Size, void *SourceInit, void *DestInit)
{
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;

    while(Size--) {*Dest++ = *Source++;}
}

//
//
//
#include "view_tilemap_file_formats.h"
#include "view_tilemap_intrinsics.h"
#include "view_tilemap_math.h"
#include "view_tilemap_sim_region.h"
#include "view_tilemap_render_group.h"
#include "view_tilemap_random.h"
#include "view_tilemap_asset.h"
#include "view_tilemap_world.h"

struct ground_buffer
{
    // NOTE(casey): An invalid P tells us that this ground_buffer has not been filled 
    world_position P; // NOTE(casey): This is the center of the bitmap
    loaded_bitmap Bitmap;
};

enum tile_attribute
{
    TileAttribute_Biome,
    TileAttribute_State,
    TileAttribute_MainSurface,
    TileAttribute_MergeSurface,
};

static const char *TileAtributes[] =
{
    "Biome",
    "State",
    "MainSurface",
    "MergeSurface",

};

struct game_state
{
    memory_arena WorldArena;
    world *World;

    real32 TypicalFloorHeight;

    world_position CameraP;
    world_position CameraBoundsMin;
    world_position CameraBoundsMax;

    b32 ViewTile;
    u32 BiomeMask;
    u32 TileStateMask;
    u32 TileSurfaceMainMask;
    u32 TileSurfaceMergeMask;
    // NOTE(paul): 00 - tile_biome_type
    //             00 - tile_state
    //             00 - tile_surface(Main)
    //             00 - tile_surface(Merge) 
    u32 ChoosenTileAttributes;
    u32 AttributeToChange;
    
    u32 WorldTileCount;
    world_tile *WorldTiles;
};

struct task_with_memory
{
    bool32 BeingUsed;
    memory_arena Arena;

    temporary_memory MemoryFlush;
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena TranArena;

    task_with_memory Tasks[4];

    game_assets *Assets;
    
    uint32 GroundBufferCount;
    ground_buffer *GroundBuffers;

    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;
};

global_variable platform_api Platform;

internal task_with_memory *BeginTaskWithMemory(transient_state *TranState);
internal void EndTaskWithMemory(task_with_memory *Task);

#define VIEW_TILEMAP_H
#endif
