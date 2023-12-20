/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

#if 0
struct unpacked_tile_attributes
{
    s32 Biome;
    s32 State;
    s32 MainSurface;
    s32 MergeSurface;
};

inline unpacked_tile_attributes
UnpackTileAttributes(game_state *GameState, u32 TileAttributes)
{
    unpacked_tile_attributes Result = {};

    Result.Biome = (TileAttributes & GameState->BiomeMask) >> 24;
    Result.State = (TileAttributes & GameState->TileStateMask) >> 16;
    Result.MainSurface = (TileAttributes & GameState->TileSurfaceMainMask) >> 8;
    Result.MergeSurface = (TileAttributes & GameState->TileSurfaceMergeMask);

    return(Result);
}

inline u32
PackTileAttributes(unpacked_tile_attributes Attributes)
{
    u32 Result = ((Attributes.Biome << 24) |
                  (Attributes.State << 16) |
                  (Attributes.MainSurface << 8) |
                  (Attributes.MergeSurface));

    return(Result);
}

inline u32
PackTileAttributes(u32 Biome, u32 State, u32 MainSurface, u32 MergeSurface)
{
    u32 Result = ((Biome << 24) |
                  (State << 16) |
                  (MainSurface << 8) |
                  (MergeSurface));

    return(Result);
}

inline void
DrawTilesForAttributes(render_group *RenderGroup, unpacked_tile_attributes Attributes)
{
    r32 TileSize = 100.0f;
    r32 TileHalfSize = TileSize / 2.0f;
    r32 Offset = 90.0f;
    r32 OffsetY = AtY - Offset;
    r32 OffsetX = LeftEdge + Offset;
        
    bitmap_id ID = GetTileBitmapIDForAttributes(RenderGroup->Assets, Attributes.Biome, Attributes.State,
                                                Attributes.MainSurface, Attributes.MergeSurface);
    PushRectOutline(RenderGroup, V3(OffsetX + TileHalfSize, OffsetY + TileHalfSize, 0),
                    V2(TileSize, TileSize), V4(1, 0, 0, 1), 2.0f);
    PushBitmap(RenderGroup, ID, TileSize, V3(OffsetX, OffsetY, 0));
    OffsetX += Offset + TileSize;

    ID = GetSolidTileBitmapIDForAttributes(RenderGroup->Assets, Attributes.Biome, Attributes.MainSurface);
    PushRectOutline(RenderGroup, V3(OffsetX + TileHalfSize, OffsetY + TileHalfSize, 0),
                    V2(TileSize, TileSize), V4(1, 0, 0, 1), 2.0f);
    PushBitmap(RenderGroup, ID, TileSize, V3(OffsetX, OffsetY, 0));
    OffsetX += Offset + TileSize;

    ID = GetSolidTileBitmapIDForAttributes(RenderGroup->Assets, Attributes.Biome, Attributes.MergeSurface);
    PushRectOutline(RenderGroup, V3(OffsetX + TileHalfSize, OffsetY + TileHalfSize, 0),
                    V2(TileSize, TileSize), V4(1, 0, 0, 1), 2.0f);
    PushBitmap(RenderGroup, ID, TileSize, V3(OffsetX, OffsetY, 0));
    OffsetX += Offset + TileSize;

    AtY -= 120.0f;
}

internal void
ShowCurrentTileAttributes(render_group *RenderGroup, game_state *GameState, world_position MouseChunkP)
{
    if((MouseChunkP.ChunkX >= 0) && (MouseChunkP.ChunkY >= 0))
    {
        world_tile *WorldTile = GetTileFromChunkPosition(GameState, MouseChunkP);

        unpacked_tile_attributes TileAttributes = UnpackTileAttributes(GameState, WorldTile->TileAttributes);

        DEBUGTextLine(RenderGroup, "CurrentTileAttributes:");
        char TextBuffer[256];
        _snprintf_s(TextBuffer, sizeof(TextBuffer),
                    "Biome: %s / State: %s / MainSurface: %s / MergeSurface: %s",
                    Biomes[TileAttributes.Biome], States[TileAttributes.State],
                    Surfaces[TileAttributes.MainSurface], Surfaces[TileAttributes.MergeSurface]);
        DEBUGTextLine(RenderGroup, TextBuffer);

        DrawTilesForAttributes(RenderGroup, TileAttributes);
    }
}

internal void
ShowChoosenTileAttributes(render_group *RenderGroup, game_state *GameState)
{
    unpacked_tile_attributes TileAttributes = UnpackTileAttributes(GameState, GameState->ChoosenTileAttributes);

    DEBUGTextLine(RenderGroup, "ChoosenTileAttributes:");
    char TextBuffer[256];
    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "Biome: %s / State: %s / MainSurface: %s / MergeSurface: %s",
                Biomes[TileAttributes.Biome], States[TileAttributes.State],
                Surfaces[TileAttributes.MainSurface], Surfaces[TileAttributes.MergeSurface]);
    DEBUGTextLine(RenderGroup, TextBuffer);

    DrawTilesForAttributes(RenderGroup, TileAttributes);
}

inline void
ChangeChoosenAttributeValueFor(game_state *GameState, b32 Up)
{
    unpacked_tile_attributes Attributes = UnpackTileAttributes(GameState, GameState->ChoosenTileAttributes);
    s32 Addend = Up ? 1 : -1;
    switch(GameState->AttributeToChange)
    {
        case TileAttribute_Biome:
        {
            Attributes.Biome += Addend;
            if(Attributes.Biome < 0) {Attributes.Biome = BiomeType_Count - 1;}
            if(Attributes.Biome >= BiomeType_Count) {Attributes.Biome = 0;}
        } break;

        case TileAttribute_State:
        {
            Attributes.State += Addend;
            if(Attributes.State < 0) {Attributes.State = TileState_Count - 1;}
            if(Attributes.State >= TileState_Count) {Attributes.State = 0;}
        } break;

        case TileAttribute_MainSurface:
        {
            Attributes.MainSurface += Addend;
            if(Attributes.MainSurface < 0) {Attributes.MainSurface = TileSurface_Count - 1;}
            if(Attributes.MainSurface >= TileSurface_Count) {Attributes.MainSurface = 0;}
        } break;

        case TileAttribute_MergeSurface:
        {
            Attributes.MergeSurface += Addend;
            if(Attributes.MergeSurface < 0) {Attributes.MergeSurface = TileSurface_Count - 1;}
            if(Attributes.MergeSurface >= TileSurface_Count) {Attributes.MergeSurface = 0;}
        } break;
    }

    GameState->ChoosenTileAttributes = PackTileAttributes(Attributes);
}
#endif

inline bitmap_id
GetTileBitmapIDForAttributes(game_assets *Assets, u32 Biome, u32 State, u32 MainSurface, u32 MergeSurface)
{
    bitmap_id Result = {};
    
    asset_vector WeightVector = {};
    WeightVector.E[Tag_TileBiomeType] = 1.0f;
    WeightVector.E[Tag_TileState] = 1.0f;
    WeightVector.E[Tag_TileMainSurface] = 1.0f;
    WeightVector.E[Tag_TileMergeSurface] = 1.0f;

    asset_vector MatchVector = {};
    MatchVector.E[Tag_TileBiomeType] = (r32)Biome;
    MatchVector.E[Tag_TileState] = (r32)State;
    MatchVector.E[Tag_TileMainSurface] = (r32)MainSurface;
    MatchVector.E[Tag_TileMergeSurface] = (r32)MergeSurface;

    Result = GetBestMatchBitmapFrom(Assets, Asset_Tile,
                                    &MatchVector, &WeightVector);

    return(Result);
}

inline bitmap_id
GetSolidTileBitmapIDForAttributes(game_assets *Assets, u32 Biome, u32 Surface)
{
    bitmap_id Result = {};
    Result = GetTileBitmapIDForAttributes(Assets, Biome, TileState_Solid, Surface, Surface);

    return(Result);
}

inline v2
TopDownAlign(loaded_bitmap *Bitmap, v2 Align)
{
    Align.y = (real32)(Bitmap->Height - 1) - Align.y;

    Align.x = SafeRatio0(Align.x, (real32)Bitmap->Width);
    Align.y = SafeRatio0(Align.y, (real32)Bitmap->Height);
    
    return(Align);
}

internal void
DEBUGWriteBMP(transient_state *TranState, thread_context *Thread, debug_platform_write_entire_file *WriteEntireFile, char *FileName,
              bitmap_header *Header, void *BitmapMemory, uint32 BitmapMemorySize)
{
    temporary_memory WriteMemory = BeginTemporaryMemory(&TranState->TranArena);

    uint32 ContentSize = sizeof(bitmap_header) + BitmapMemorySize;
    void *Content = PushSize(&TranState->TranArena, ContentSize);

    MemoryCopy(Content, Header, sizeof(bitmap_header));
    MemoryCopy((uint8 *)Content + sizeof(bitmap_header), BitmapMemory, BitmapMemorySize);

    WriteEntireFile(Thread, FileName, ContentSize, Content);
    
    EndTemporaryMemory(WriteMemory);
}

inline void
LoadMapAndCameraBounds(game_state *GameState, thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName)
{
    map_bitmap *MapBitmap = &GameState->MapBitmap;

    bitmap_header *Header = &MapBitmap->Header;
    MapBitmap->Bitmap = DEBUGLoadBMP(Thread, ReadEntireFile, FileName, 0, 0, Header, true);
    MapBitmap->BitmapSize = MapBitmap->Bitmap.Height * MapBitmap->Bitmap.Width * BITMAP_BYTES_PER_PIXEL;
    
    GameState->RotationAndFlipState = PushArray(&GameState->WorldArena,
                                                MapBitmap->Bitmap.Height * MapBitmap->Bitmap.Width,
                                                uint8);
#if 0
    loaded_bitmap *Bitmap = &MapBitmap->Bitmap;
    for(int32 I = 0;
        I < MapBitmap->Bitmap.Height * MapBitmap->Bitmap.Width;
        ++I)
    {
        uint32 *Pixel = (uint32 *)Bitmap->Memory + I;
        *Pixel |= 0x0f000000;
        *Pixel &= 0xfffffff8;
    }
#endif

    GameState->CameraBoundsMin.ChunkX = 0;
    GameState->CameraBoundsMin.ChunkY = 0;
    GameState->CameraBoundsMin.ChunkZ = 0;
    
    GameState->CameraBoundsMax.ChunkX = MapBitmap->Bitmap.Width / TILES_PER_CHUNK;
    GameState->CameraBoundsMax.ChunkY = MapBitmap->Bitmap.Height / TILES_PER_CHUNK;
    GameState->CameraBoundsMax.ChunkZ = 0;
}

internal loaded_bitmap *
FindTileBitmapByIdentity(loaded_tile *Tiles, uint32 TileCount, uint32 Identity)
{
    loaded_bitmap *Result = 0;

    bool32 Found = false;
    for(uint32 TileIndex = 0;
        TileIndex < TileCount;
        ++TileIndex)
    {
        loaded_tile *Tile = Tiles + TileIndex;
        for(uint32 TileTypeIndex = 0;
            TileTypeIndex < ArrayCount(Tile->Identity);
            ++TileTypeIndex)
        {
            uint32 TileIdentity = Tile->Identity[TileTypeIndex];
            if(TileIdentity == Identity)
            {
                Result = Tile->Bitmap + TileTypeIndex;
                break;
            }
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


internal uint32
LoadTileDataAndIdentities(memory_arena *Arena, loaded_tile *Tiles, loaded_bitmap *TileSheets, int32 TileDim)
{
    loaded_bitmap *TileSheet0 = TileSheets + 0;
    Assert(TileSheet0->Width == TileSheet0->Height);

    uint32 TileCountX = (TileSheet0->Width / TileDim);
    uint32 TileCountY = (TileSheet0->Height / TileDim);

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
            uint64 ColorSum = 0;
            for(uint32 TileSheetIndex = 0;
                TileSheetIndex < 6;
                ++TileSheetIndex)
            {
                loaded_bitmap *TileSheet = TileSheets + TileSheetIndex;

                Tile->Bitmap[TileSheetIndex] = MakeEmptyBitmap(Arena, TileDim, TileDim);
                loaded_bitmap *Bitmap = Tile->Bitmap + TileSheetIndex;
                uint32 *Identity = Tile->Identity + TileSheetIndex;
                Bitmap->WidthOverHeight = SafeRatio0((real32)Bitmap->Width, (real32)Bitmap->Height);
                Bitmap->AlignPercentage = V2(0.0f, 0.0f);

                int32 MinX = TileX * TileDim;
                int32 MinY = TileY * TileDim;
                int32 MaxX = MinX + TileDim;
                int32 MaxY = MinY + TileDim;

                uint8 *SourceRow = ((uint8 *)TileSheet->Memory + MinX*BITMAP_BYTES_PER_PIXEL + MinY*TileSheet->Pitch);
    
                uint8 *DestRow = (uint8 *)Bitmap->Memory;
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

                        if(TileSheetIndex == 0)
                        {
                            ColorSum += *Dest;
                        }

                        ++Dest;
                        ++Source;
                    }

                    DestRow += Bitmap->Pitch;
                    SourceRow += TileSheet->Pitch;
                }
            
                *Identity = (uint32)(ColorSum / (TileDim*TileDim));
                *Identity &= 0xfffffff8;
                *Identity += TileSheetIndex;
            }
            ++LoadedTileCount;
        }
    }

    return(LoadedTileCount);
}

internal void
DrawTileIdentityAndBitmap(render_group *RenderGroup, game_state *GameState, transient_state *TranState, v3 Offset)
{
    loaded_bitmap *Map = &GameState->MapBitmap.Bitmap;

    if(Map->Memory)
    {
        world_position ChunkP = {};
        low_entity *Low = GetLowEntity(GameState, GameState->CameraFollowingEntityIndex);
        world_position TestP = GameState->CameraP; 
        if(Low)
        {
            TestP = Low->P;
        }
            
        for(uint32 GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            if(AreInSameChunk(GameState->World, &TestP, &GroundBuffer->P))
            {
                ChunkP = TestP;
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
            loaded_bitmap *Bitmap = FindTileBitmapByIdentity(GameState->Tiles, GameState->LoadedTileCount, *Identity);
            v4 Color = (1.0f / 255.0f)*V4((real32)(((*Identity >> 16) & 0xFF)),
                                          (real32)(((*Identity >> 8) & 0xFF)),
                                          (real32)(((*Identity >> 0) & 0xFF)),
                                          (real32)(((*Identity >> 24) & 0xFF)));
        
            PushRect(RenderGroup, Offset, V2(1.0f, 1.0f), Color);
            if(Bitmap)
            {
                PushBitmap(RenderGroup, Bitmap, 1.0f, Offset + V3(1.0f, -0.5f, 0.0f));
            }
        }
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
        PushBitmap(RenderGroup, Tile->Bitmap, TileDim, V3(TileOffset, 0.0f));

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

inline void
ChangeGroundTileTexture(game_state *GameState)
{
    loaded_bitmap *Map = &GameState->MapBitmap.Bitmap;
    loaded_tile *Tile = GameState->Tiles + GameState->Cursor.TileIndex;

    uint32 *TileToChange = (uint32 *)Map->Memory + GameState->TileIndexInMap;
    *TileToChange = Tile->Identity[0];
}

inline void
ChangeGroundTileTexture(game_state *GameState, bool32 Flip, bool32 Rotate)
{
    loaded_bitmap *Map = &GameState->MapBitmap.Bitmap;

    uint32 *TileToChange = (uint32 *)Map->Memory + GameState->TileIndexInMap;
    uint32 CheckForRotationOrFlip = *TileToChange & 0x00000007;
    uint32 DefaultIdentity = *TileToChange & 0xfffffff8;
    
    if(Flip)
    {
        if(CheckForRotationOrFlip == 4)
        {
            *TileToChange = DefaultIdentity + 5;
        }
        else if(CheckForRotationOrFlip == 5)
        {
            *TileToChange = DefaultIdentity;
        }
        else
        {
            *TileToChange = DefaultIdentity + 4;
        }
    }

    if(Rotate)
    {
        if(CheckForRotationOrFlip == 1)
        {
            *TileToChange = DefaultIdentity + 2;
        }
        else if(CheckForRotationOrFlip == 2)
        {
            *TileToChange = DefaultIdentity + 3;
        }
        else if(CheckForRotationOrFlip == 3)
        {
            *TileToChange = DefaultIdentity;
        }
        else
        {
            *TileToChange = DefaultIdentity + 1;
        }
    }
}
