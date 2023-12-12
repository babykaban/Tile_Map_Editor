/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   ======================================================================== */

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX / 64) 
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

#define WORLD_WIDTH_TILE_COUNT 512 
#define WORLD_HEIGHT_TILE_COUNT 512 

#define TILES_PER_CHUNK 4

inline world_position
NullPosition(void)
{
    world_position Result = {};

    Result.ChunkX = TILE_CHUNK_UNINITIALIZED;

    return(Result);
}

inline bool32
IsValid(world_position P)
{
    bool32 Result = (P.ChunkX != TILE_CHUNK_UNINITIALIZED);
    return(Result);
}

inline bool32
IsCanonical(real32 ChunkDim, real32 TileRel)
{
    // TODO(casey): Fix floating point math so this can be exact?
    real32 Epsilon = 0.01f;
    bool32 Result = ((TileRel >= -(0.5f*ChunkDim + Epsilon)) &&
                     (TileRel <= (0.5f*ChunkDim + Epsilon)));

    return(Result);
}

inline bool32
IsCanonical(world *World, v2 Offset)
{
    bool32 Result = (IsCanonical(World->ChunkDimInMeters.x, Offset.x) &&
                     IsCanonical(World->ChunkDimInMeters.y, Offset.y));

    return(Result);
}

inline bool32
AreInSameChunk(world *World, world_position *A, world_position *B)
{       
    Assert(IsCanonical(World, A->Offset_));
    Assert(IsCanonical(World, B->Offset_));

    bool32 Result = ((A->ChunkX == B->ChunkX) &&
                     (A->ChunkY == B->ChunkY));
    return(Result);
}

inline world_chunk *
GetWorldChunk(world *World, int32 ChunkX, int32 ChunkY, memory_arena *Arena = 0)
{
    Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
        
    
    // TODO(casey): BETTER HASH FUNCTION!!!!
    uint32 HashValue = 19*ChunkX + 7*ChunkY;
    uint32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
    Assert(HashSlot < ArrayCount(World->ChunkHash));

    world_chunk *Chunk = World->ChunkHash + HashSlot;
    do
    {
        if((ChunkX == Chunk->ChunkX) &&
           (ChunkY == Chunk->ChunkY))
        {
            break;
        }

        if(Arena && (Chunk->ChunkX != TILE_CHUNK_UNINITIALIZED) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, world_chunk);
            Chunk = Chunk->NextInHash;
            Chunk->ChunkX = TILE_CHUNK_UNINITIALIZED;
        }
        
        if(Arena && (Chunk->ChunkX == TILE_CHUNK_UNINITIALIZED))
        {
            Chunk->ChunkX = ChunkX;
            Chunk->ChunkY = ChunkY;

            Chunk->NextInHash = 0;            
            break;
        }

        Chunk = Chunk->NextInHash;
    } while(Chunk);
    
    return(Chunk);
}

internal void
InitializeWorld(world *World, v2 ChunkDimInMeters)
{
    World->ChunkDimInMeters = ChunkDimInMeters;
    
    for(uint32 ChunkIndex = 0;
        ChunkIndex < ArrayCount(World->ChunkHash);
        ++ChunkIndex)
    {
        World->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
    }
}

inline void
RecanonicalizeCoord(real32 ChunkDim, int32 *Tile, real32 *TileRel)
{
    int32 Offset = RoundReal32ToInt32(*TileRel / ChunkDim);
    *Tile += Offset;
    *TileRel -= Offset*ChunkDim;

    Assert(IsCanonical(ChunkDim, *TileRel));
}

inline world_position
MapIntoChunkSpace(world *World, world_position BasePos, v2 Offset)
{
    world_position Result = BasePos;

    Result.Offset_ += Offset;
    RecanonicalizeCoord(World->ChunkDimInMeters.x, &Result.ChunkX, &Result.Offset_.x);
    RecanonicalizeCoord(World->ChunkDimInMeters.y, &Result.ChunkY, &Result.Offset_.y);
    
    return(Result);
}

inline v2
Subtract(world *World, world_position *A, world_position *B)
{
    v2 dTile =
        {
            (real32)A->ChunkX - (real32)B->ChunkX,
            (real32)A->ChunkY - (real32)B->ChunkY
        };
    
    v2 Result = Hadamard(World->ChunkDimInMeters, dTile)  + (A->Offset_ - B->Offset_);

    return(Result);
}

inline world_position
CenteredChunkPoint(uint32 ChunkX, uint32 ChunkY)
{
    world_position Result = {};

    Result.ChunkX = ChunkX;
    Result.ChunkY = ChunkY;

    return(Result);
}

inline world_position
CenteredChunkPoint(world_chunk *Chunk)
{
    world_position Result = CenteredChunkPoint(Chunk->ChunkX,
                                               Chunk->ChunkY);

    return(Result);
}

