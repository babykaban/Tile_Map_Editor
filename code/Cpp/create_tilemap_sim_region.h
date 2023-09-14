#if !defined(SPELLWEAVER_SIM_REGION_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

enum entity_type
{
    EntityType_Null,
    EntityType_Camera,
};

struct sim_entity
{
    // NOTE(casey): This are only for the sim region
    uint32 StorageIndex;
    bool32 Updatable;
    
    //
    
    entity_type Type;
    v3 P;
    v3 dP;
    int32  dAbsTileZ;
    
};

struct sim_entity_hash
{
    sim_entity *Ptr;
    uint32 Index;
};

struct sim_region
{
    // TODO(casey): Need a hash table here to map stored entity indices
    // to sim entities!

    world *World;
    real32 MaxEntityRadius;
    real32 MaxEntityVelocity;
    
    world_position Origin;
    rectangle3 Bounds;
    rectangle3 UpdatableBounds;

    uint32 MaxEntityCount;
    uint32 EntityCount;
    sim_entity *Entities;

    real32 GroundZBase;
    
    // NOTE(casey): Must be a power of two!
    sim_entity_hash Hash[4096];
};

#define SPELLWEAVER_SIM_REGION_H
#endif
