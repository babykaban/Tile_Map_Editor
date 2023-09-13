#if !defined(CREATE_TILEMAP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

/*
  TODO(casey):

  ARCHITECTURE EXPLORATION
  - Z!
    - How is this rendered?
      "Frinstances"!
  - Collision detection?
    - Clean up predicate proliferation! Can we make a nice clean
      set of flags/rules so that it's easy to understand how
      things work in terms of special handling? This may involve
      making the iteration handle everything instead of handling
      overlap outside and so on.
    - Transient collision rules! Ckear based on flag.
      - Allow non-transient rules to everride transient ones.
      - Entry/exit?
    - What's the plan for robustness / shape definition?
    - (Implement reprojection to handle interpenetration)
  - Implement multiple sim regions per frame
    - Per-entity clocking
    - Sim region merging? For multiple players?

  - Debug code
    - Logging
    - Diagramming
    - (A LITTLE GUI, but only a little!) Switches / sliders / etc.
    - Draw tile chunks so we can verify that things are aligned /
      in the chunks we want them to be in / etc.
    
  - Audio
    - Sound effect triggers
    - Ambient sounds
    - Music
  - Asset streaming
    
  - Metagame / save game?
    - How do you enter "save slot"?
    - Persistent unlocks/etc.
    - Do we allow saved games? Probably yes, just only for "pausing".
    * Continuos save for crash recovery?
  - Rudimentary world gen (no quality, just "what sorts of things" we do)
    - Placement of background things
    - Connectivity?
    - Non-overlapping?
      - Map display
        - Magnets - how they work??
  - AI
    - Rudimentary monstar behavior example
    * Pathfinding
    - AI "storage"

  * Animation, should probably lead into rendering
    - Skeletal animation
    - Particle systems
    
  PRODACTION
  - Rendering
  -> GAME
    - Entity system
    - World generation
 */

#include "create_tilemap_platform.h"

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

#include "create_tilemap_intrinsics.h"
#include "create_tilemap_math.h"
#include "create_tilemap_world.h"
#include "create_tilemap_sim_region.h"
#include "create_tilemap_entity.h"
#include "create_tilemap_render_group.h"

struct sprite_sheet
{
    uint32 SpriteCount;
    loaded_bitmap Sprites[32];
};

struct hero_bitmaps
{
    sprite_sheet HeroSprites[4];
    loaded_bitmap Spheres[3]; 
    loaded_bitmap Shadow;
};

struct low_entity
{
    world_position P;
    sim_entity Sim;
};

struct controlled_hero
{
    uint32 EntityIndex;
    // NOTE(casey): These are the controller requests for simulation
    v2 ddP;
    v2 dSword;
    real32 dZ;
};

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32 StorageIndexA;
    uint32 StorageIndexB;

    pairwise_collision_rule *NextInHash;
};

struct game_state;
internal void AddCollisionRule(game_state *GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool32 CanCollide);
internal void ClearCollisionRulesFor(game_state *GameState, uint32 StorageIndex);

struct ground_buffer
{
    // NOTE(casey): An invalid P tells us that this ground_buffer has not been filled 
    world_position P; // NOTE(casey): This is the center of the bitmap
    loaded_bitmap Bitmap;
};

struct game_state
{
    memory_arena WorldArena;
    world *World;

    real32 TypicalFloorHeight;
    
    // TODO(casey): Should we allow split-screen?
    uint32 CameraFollowingEntityIndex;
    world_position CameraP;

    controlled_hero ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];

    // TODO(casey): Change the name to "Stored entity"
    uint32 LowEntityCount;
    low_entity LowEntities[100000];

    hero_bitmaps HeroBitmaps;

    loaded_bitmap Border;
    loaded_bitmap Grass;
    
    // TODO(casey): Must be power of two
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    sim_entity_collision_volume_group *NullCollision;
    sim_entity_collision_volume_group *SwordCollision;
    sim_entity_collision_volume_group *StairCollision;
    sim_entity_collision_volume_group *PlayerCollision;
    sim_entity_collision_volume_group *MonstarCollision;
    sim_entity_collision_volume_group *FamiliarCollision;
    sim_entity_collision_volume_group *WallCollision;
    sim_entity_collision_volume_group *StandardRoomCollision;

    real32 Time;

    loaded_bitmap TestDiffuse;
    loaded_bitmap TestNormal;
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena TranArena;

    uint32 GroundBufferCount;
    ground_buffer *GroundBuffers;

    platform_work_queue *RenderQueue;
    
    uint32 EnvMapWidth;
    uint32 EnvMapHeight;
    // NOTE(casey): 0 is bottom, 1 is middle, 2 is top
    environment_map EnvMaps[3];
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

#define CREATE_TILEMAP_H
#endif
