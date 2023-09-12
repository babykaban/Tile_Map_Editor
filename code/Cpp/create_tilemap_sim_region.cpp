/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

internal sim_entity_hash *
GetHashFromStorageIndex(sim_region *SimRegion, uint32 StorageIndex)
{
    Assert(StorageIndex);

    sim_entity_hash *Result = 0;
    uint32 HashValue = StorageIndex;
    for(uint32 Offset = 0;
        Offset < ArrayCount(SimRegion->Hash);
        ++Offset)
    {
        uint32 HashMask = (ArrayCount(SimRegion->Hash) - 1);
        uint32 HashIndex = ((HashValue + Offset) & HashMask);
        sim_entity_hash *Entry = SimRegion->Hash + HashIndex;

        if((Entry->Index == 0) || (Entry->Index == StorageIndex))
        {
            Result = Entry;
            break;
        }
    }

    return(Result);
}

inline sim_entity *
GetEntityByStorageIndex(sim_region *SimRegion, uint32 StorageIndex)
{
    sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
    sim_entity *Result = Entry->Ptr;

    return(Result);
}

inline v3
GetSimSpaceP(sim_region *SimRegion, low_entity *Stored)
{
    // NOTE(casey): Map the entity into camera space
    // TODO(casey): Do we want to set this to signaling NAN in
    // debug mode to make sure nobody ever uses the position
    // of a nonspatial entity?
    v3 Result = InvalidP;
    if(!IsSet(&Stored->Sim, EntityFlag_Nonspatial))
    {
        Result = Subtract(SimRegion->World, &Stored->P, &SimRegion->Origin);
    }

    return(Result);
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source, v3 *SimP);
inline void
LoadEntityreference(game_state *GameState, sim_region *SimRegion, entity_reference *Ref)
{
    if(Ref->Index)
    {
        sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, Ref->Index); 
        if(Entry->Ptr == 0)
        {
            Entry->Index = Ref->Index;
            low_entity *LowEntity = GetLowEntity(GameState, Ref->Index);
            v3 P = GetSimSpaceP(SimRegion, LowEntity);
            Entry->Ptr = AddEntity(GameState, SimRegion, Ref->Index, LowEntity, &P);
        }
        
        Ref->Ptr = Entry->Ptr;
    }
}

inline void
StoreEntityreference(entity_reference *Ref)
{
    if(Ref->Ptr != 0)
    {
        Ref->Index = Ref->Ptr->StorageIndex;
    }
}

internal sim_entity *
AddEntityRaw(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source)
{
    Assert(StorageIndex);
    sim_entity *Entity = 0;
    
    sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex); 
    if(Entry->Ptr == 0)
    {
        if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
        {
            Entity = SimRegion->Entities + SimRegion->EntityCount++;

            Entry->Index = StorageIndex;
            Entry->Ptr = Entity;

            if(Source)
            {
                // TODO(casey): This should really be a decompression step, not
                // a copy!
                *Entity = Source->Sim;
                LoadEntityreference(GameState, SimRegion, &Entity->Sword);

                Assert(!IsSet(&Source->Sim, EntityFlag_Simming));
                AddFlags(&Source->Sim, EntityFlag_Simming);
            }

            Entity->StorageIndex = StorageIndex;
            Entity->Updatable = false;
        }
        else
        {
            InvalidCodePath;
        }
    }

    return(Entity);
}

inline bool32
EntityOverlapsRectangle(v3 P, sim_entity_collision_volume Volume, rectangle3 Rect)
{
    rectangle3 Grown = AddRadiusTo(Rect, 0.5f*Volume.Dim);
    bool32 Result = IsInRectangle(Grown, P+ Volume.OffsetP);
    return(Result);
}

internal sim_entity *
AddEntity(game_state *GameState, sim_region *SimRegion, uint32 StorageIndex, low_entity *Source, v3 *SimP)
{
    sim_entity *Dest = AddEntityRaw(GameState, SimRegion, StorageIndex, Source);
    if(Dest)
    {
        if(SimP)
        {
            Dest->P = *SimP;
            Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume,
                                                      SimRegion->UpdatableBounds);
        }
        else
        {
            Dest->P = GetSimSpaceP(SimRegion, Source);
        }
    }

    return(Dest);
}

internal void
EndSim(sim_region *Region, game_state *GameState)
{
    // TODO(casey): Maybe don't take a game state here, low entities should be stored
    // in the world?

    sim_entity *Entity = Region->Entities;
    for(uint32 EntityIndex = 0;
        EntityIndex < Region->EntityCount;
        ++EntityIndex, ++Entity)
    {
        low_entity *Stored = GameState->LowEntities + Entity->StorageIndex;

        Assert(IsSet(&Stored->Sim, EntityFlag_Simming));
        Stored->Sim = *Entity;
        Assert(!IsSet(&Stored->Sim, EntityFlag_Simming));

        StoreEntityreference(&Stored->Sim.Sword);
        
        // TODO(casey): Save state back to the stored entity, once high entities
        // do state decompression, etc.

        world_position NewP = IsSet(Entity, EntityFlag_Nonspatial) ?
            NullPosition() :
            MapIntoChunkSpace(GameState->World, Region->Origin, Entity->P);

        ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity->StorageIndex,
                             Stored, NewP);
    }
}

internal void
MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity, real32 dt,
           move_spec *MoveSpec, v3 ddP)
{
}
