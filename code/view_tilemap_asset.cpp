/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */


enum finalize_asset_operation
{
    FinalizeAsset_None,
    FinalizeAsset_Font,
};

struct load_asset_work
{
    task_with_memory *Task;
    asset *Asset;

    platform_file_handle *Handle;
    u64 Offset;
    u64 Size;
    void *Destination;

    finalize_asset_operation FinalizeOperation;
    u32 FinalState;
};

internal void
LoadAssetWorkDirectly(load_asset_work *Work)
{
    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);

    if(PlatformNoFileErrors(Work->Handle))
    {
        switch(Work->FinalizeOperation)
        {
            case FinalizeAsset_None:
            {
                // NOTE(casey): Nothing to do.
            } break;

            case FinalizeAsset_Font:
            {
                loaded_font *Font = &Work->Asset->Header->Font;
                ssa_font *SSA = &Work->Asset->SSA.Font;
                for(u32 GlyphIndex = 1;
                    GlyphIndex < SSA->GlyphCount;
                    ++GlyphIndex)
                {
                    ssa_font_glyph *Glyph = Font->Glyphs + GlyphIndex;
                    Assert(Glyph->UnicodeCodePoint < SSA->OnePastHighestCodePoint);
                    Assert((u32)(u16)GlyphIndex == GlyphIndex);
                    Font->UnicodeMap[Glyph->UnicodeCodePoint] = (u16)GlyphIndex;
                }

            } break;
        }
    }    

    CompletePreviousWritesBeforeFutureWrites;

    if(!PlatformNoFileErrors(Work->Handle))
    {
        ZeroSize(Work->Size, Work->Destination);
    }

    Work->Asset->State = Work->FinalState;
}

internal
PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
{
    load_asset_work *Work = (load_asset_work *)Data;

    LoadAssetWorkDirectly(Work);
    
    EndTaskWithMemory(Work->Task);
}

inline asset_file *
GetFile(game_assets *Assets, u32 FileIndex)
{
    Assert(FileIndex < Assets->FileCount);

    asset_file *Result = Assets->Files + FileIndex;

    return(Result);
}

inline platform_file_handle *
GetFileHandleFor(game_assets *Assets, u32 FileIndex)
{
    Assert(FileIndex < Assets->FileCount);

    platform_file_handle *Handle = &GetFile(Assets, FileIndex)->Handle;

    return(Handle);
}

internal asset_memory_block *
InsertBlock(asset_memory_block *Prev, memory_index Size, void *Memory)
{
    Assert(Size > sizeof(asset_memory_block));
    asset_memory_block *Block = (asset_memory_block *)Memory;
    Block->Flags = 0;
    Block->Size = Size - sizeof(asset_memory_block);

    Block->Prev = Prev;
    Block->Next = Prev->Next;

    Block->Prev->Next = Block;
    Block->Next->Prev = Block;

    return(Block);
}

internal asset_memory_block *
FindBlockForSize(game_assets *Assets, memory_index Size)
{
    asset_memory_block *Result = 0;

    for(asset_memory_block *Block = Assets->MemorySentinel.Next;
        Block != &Assets->MemorySentinel;
        Block = Block->Next)
    {
        if(!(Block->Flags & AssetMemory_Used))
        {
            if(Block->Size >= Size)
            {
                Result = Block;
                break;
            }
        }
    }

    return(Result);
}

internal b32
MergeIfPossible(game_assets *Assets, asset_memory_block *First, asset_memory_block *Second)
{
    b32 Result = false;
    if((First != &Assets->MemorySentinel) && (Second != &Assets->MemorySentinel))
    {
        if(!(First->Flags & AssetMemory_Used) && !(Second->Flags & AssetMemory_Used))
        {
            u8 *ExpectedSecond = (u8 *)First + sizeof(asset_memory_block) + First->Size;
            if((u8 *)Second == ExpectedSecond)
            {
                Second->Next->Prev = Second->Prev;
                Second->Prev->Next = Second->Next;
                First->Size += sizeof(asset_memory_block) + Second->Size;

                Result = true;
            }
        }
    }

    return(Result);
}

internal b32
GenerationHasCompleted(game_assets *Assets, u32 CheckID)
{
    b32 Result = true;

    for(u32 InFlightGenerationIndex = 0;
        InFlightGenerationIndex < Assets->InFlightGenerationCount;
        ++InFlightGenerationIndex)
    {
        if(Assets->InFlightGenerations[InFlightGenerationIndex] == CheckID)
        {
            Result = false;
            break;
        }
    }

    return(Result);
}

internal asset_memory_header *
AcquireAssetMemory(game_assets *Assets, u32 Size, u32 ID)
{
    asset_memory_header *Result = 0;

    BeginAssetLock(Assets);

    asset_memory_block *Block = FindBlockForSize(Assets, Size);
    for(;;)
    {
        if(Block && (Size <= Block->Size))
        {
            Block->Flags |= AssetMemory_Used;
            Result = (asset_memory_header *)(Block + 1);

            memory_index RemainingSize = Block->Size - Size;
            memory_index BlockSplitThreashold = 4096;
            if(RemainingSize > BlockSplitThreashold)
            {
                Block->Size -= RemainingSize;
                InsertBlock(Block, RemainingSize, (u8 *)Result + Size);
            }
            
            break;
        }
        else
        {
            for(asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
                Header != &Assets->LoadedAssetSentinel;
                Header = Header->Prev)
            {
                u32 AssetIndex = Header->AssetIndex;
                asset *Asset = Assets->Assets + AssetIndex;
                if((Asset->State == AssetState_Loaded) &&
                   (GenerationHasCompleted(Assets, Asset->Header->GenerationID)))
                {

                    Assert(Asset->State == AssetState_Loaded);
    
                    RemoveAssetHeaderFromList(Header);

                    Block = (asset_memory_block *)Asset->Header - 1;
                    Block->Flags &= ~AssetMemory_Used;

                    if(MergeIfPossible(Assets, Block->Prev, Block))
                    {
                        Block = Block->Prev;
                    }

                    MergeIfPossible(Assets, Block, Block->Next);
                    
                    Asset->State = AssetState_Unloaded;
                    Asset->Header = 0;

                    break;
                }
            }
        }
    }

    if(Result)
    {
        Result->AssetIndex = ID;
        Result->TotalSize = Size;
        InsertAssetHeaderAtFront(Assets, Result);
    }

    EndAssetLock(Assets);

    return(Result);
}

struct asset_memory_size
{
    u32 Total;
    u32 Data;
    u32 Section;
};

internal void
LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate)
{
    asset *Asset = Assets->Assets + ID.Value;
    if(ID.Value)
    {
        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
           AssetState_Unloaded)
        {
            task_with_memory *Task = 0;

            if(!Immediate)
            {
                Task = BeginTaskWithMemory(Assets->TranState);
            }
        
            if(Immediate || Task)
            {
                ssa_bitmap *Info = &Asset->SSA.Bitmap;

                asset_memory_size Size = {};
                u32 Width = Info->Dim[0];
                u32 Height = Info->Dim[1];
                Size.Section = 4*Width;
                Size.Data = Height*Size.Section;
                Size.Total = Size.Data + sizeof(asset_memory_header);

                Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value);

                loaded_bitmap *Bitmap = &Asset->Header->Bitmap;
                Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
                Bitmap->Width = Info->Dim[0];
                Bitmap->Height = Info->Dim[1];
                Bitmap->Pitch = Size.Section;
                Bitmap->Memory = (Asset->Header + 1);

                load_asset_work Work;
                Work.Task = Task;
                Work.Asset = Assets->Assets + ID.Value;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Offset = Asset->SSA.DataOffset;
                Work.Size = Size.Data;
                Work.Destination = Bitmap->Memory;
                Work.FinalizeOperation = FinalizeAsset_None;
                Work.FinalState = AssetState_Loaded;

                if(Task)
                {
                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work);
                    *TaskWork = Work;
                
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                }
                else
                {
                    LoadAssetWorkDirectly(&Work);
                }
            }
            else
            {
                Asset->State = AssetState_Unloaded;
            }
        }
        else if(Immediate)
        {
            asset_state volatile *State = (asset_state volatile *)&Asset->State;
            while(*State == AssetState_Queued) {}
        }
    }
}

internal void
LoadTileset(game_assets *Assets, tileset_id ID, b32 Immediate)
{
    asset *Asset = Assets->Assets + ID.Value;
    if(ID.Value)
    {
        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
           AssetState_Unloaded)
        {
            task_with_memory *Task = 0;

            if(!Immediate)
            {
                Task = BeginTaskWithMemory(Assets->TranState);
            }
        
            if(Immediate || Task)
            {
                ssa_tileset *Info = &Asset->SSA.Tileset;

                u32 TilesSize = sizeof(ssa_tile)*Info->TileCount;
                u32 SizeData = TilesSize;
                u32 SizeTotal = SizeData + sizeof(asset_memory_header);

                Asset->Header = AcquireAssetMemory(Assets, SizeTotal, ID.Value);

                loaded_tileset *Tileset = &Asset->Header->Tileset;
                Tileset->BitmapIDOffset = GetFile(Assets, Asset->FileIndex)->TileBitmapIDOffset;
                Tileset->Tiles = (ssa_tile *)(Asset->Header + 1);
                
                load_asset_work Work;
                Work.Task = Task;
                Work.Asset = Assets->Assets + ID.Value;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Offset = Asset->SSA.DataOffset;
                Work.Size = SizeData;
                Work.Destination = Tileset->Tiles;
                Work.FinalizeOperation = FinalizeAsset_None;
                Work.FinalState = AssetState_Loaded;

                if(Task)
                {
                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work);
                    *TaskWork = Work;
                
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                }
                else
                {
                    LoadAssetWorkDirectly(&Work);
                }
            }
            else
            {
                Asset->State = AssetState_Unloaded;
            }
        }
        else if(Immediate)
        {
            asset_state volatile *State = (asset_state volatile *)&Asset->State;
            while(*State == AssetState_Queued) {}
        }
    }
}

internal void
LoadSound(game_assets *Assets, sound_id ID)
{
    asset *Asset = Assets->Assets + ID.Value;
    if(ID.Value &&
       (AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
        AssetState_Unloaded))
    {
        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
        if(Task)
        {
            ssa_sound *Info = &Asset->SSA.Sound;

            asset_memory_size Size = {};

            Size.Section = Info->SampleCount*sizeof(int16);
            Size.Data = Info->ChannelCount*Size.Section;
            Size.Total = Size.Data + sizeof(asset_memory_header);

            Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value); 
            
            loaded_sound *Sound = &Asset->Header->Sound;
            Sound->SampleCount = Info->SampleCount;
            Sound->ChannelCount = Info->ChannelCount;
            u32 ChannelSize = Size.Section;

            void *Memory = (Asset->Header + 1);
            int16 *SoundAt = (int16 *)Memory;
            for(u32 ChannelIndex = 0;
                ChannelIndex < Sound->ChannelCount;
                ++ChannelIndex)
            {
                Sound->Samples[ChannelIndex] = SoundAt;
                SoundAt += ChannelSize;
            }
                
            load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);
            Work->Task = Task;
            Work->Asset = Assets->Assets + ID.Value;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->SSA.DataOffset;
            Work->Size = Size.Data;
            Work->Destination = Memory;
            Work->FinalizeOperation = FinalizeAsset_None;
            Work->FinalState = (AssetState_Loaded);
            
            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else
        {
            Asset->State = AssetState_Unloaded;
        }
    }
}

internal void
LoadFont(game_assets *Assets, font_id ID, b32 Immediate)
{
    asset *Asset = Assets->Assets + ID.Value;
    if(ID.Value)
    {
        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
           AssetState_Unloaded)
        {
            task_with_memory *Task = 0;

            if(!Immediate)
            {
                Task = BeginTaskWithMemory(Assets->TranState);
            }
        
            if(Immediate || Task)
            {
                ssa_font *Info = &Asset->SSA.Font;

                u32 HorizontalAdvanceSize = sizeof(r32)*Info->GlyphCount*Info->GlyphCount;
                u32 GlyphsSize = Info->GlyphCount*sizeof(ssa_font_glyph);
                u32 UnicodeMapSize = sizeof(u16)*Info->OnePastHighestCodePoint;
                u32 SizeData = GlyphsSize + HorizontalAdvanceSize;
                u32 SizeTotal = SizeData + sizeof(asset_memory_header) + UnicodeMapSize;

                Asset->Header = AcquireAssetMemory(Assets, SizeTotal, ID.Value);

                loaded_font *Font = &Asset->Header->Font;
                Font->BitmapIDOffset = GetFile(Assets, Asset->FileIndex)->FontBitmapIDOffset;
                Font->Glyphs = (ssa_font_glyph *)(Asset->Header + 1);
                Font->HorizontalAdvance = (r32 *)((u8 *)Font->Glyphs + GlyphsSize);
                Font->UnicodeMap = (u16 *)((u8 *)Font->HorizontalAdvance + HorizontalAdvanceSize);

                ZeroSize(UnicodeMapSize, Font->UnicodeMap);
                
                load_asset_work Work;
                Work.Task = Task;
                Work.Asset = Assets->Assets + ID.Value;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Offset = Asset->SSA.DataOffset;
                Work.Size = SizeData;
                Work.Destination = Font->Glyphs;
                Work.FinalizeOperation = FinalizeAsset_Font;
                Work.FinalState = AssetState_Loaded;

                if(Task)
                {
                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work);
                    *TaskWork = Work;
                
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                }
                else
                {
                    LoadAssetWorkDirectly(&Work);
                }
            }
            else
            {
                Asset->State = AssetState_Unloaded;
            }
        }
        else if(Immediate)
        {
            asset_state volatile *State = (asset_state volatile *)&Asset->State;
            while(*State == AssetState_Queued) {}
        }
    }
}

internal uint32
GetBestMatchAssetFrom(game_assets *Assets, asset_type_id TypeID,
                      asset_vector *MatchVector, asset_vector *WeightVector)
{
    uint32 Result = 0;

    real32 BestDiff = Real32Maximum;
    asset_type *Type = Assets->AssetTypes + TypeID;
    for(uint32 AssetIndex = Type->FirstAssetIndex;
        AssetIndex < Type->OnePastLastAssetIndex;
        ++AssetIndex)
    {
        asset *Asset = Assets->Assets + AssetIndex;
        
        real32 TotalWeightedDiff = 0.0f;
        for(uint32 TagIndex = Asset->SSA.FirstTagIndex;
            TagIndex < Asset->SSA.OnePastLastTagIndex;
            ++TagIndex)
        {
            ssa_tag *Tag = Assets->Tags + TagIndex;

            real32 A = MatchVector->E[Tag->ID];
            real32 B = Tag->Value;
            real32 D0 = AbsoluteValue(A - B);
            real32 D1 = AbsoluteValue((A - Assets->TagRange[Tag->ID]*SignOf(A)) - B);
            real32 Difference = Minimum(D0, D1);

            real32 Weighted = WeightVector->E[Tag->ID]*Difference;
            TotalWeightedDiff += Weighted;
        }

        if(BestDiff > TotalWeightedDiff)
        {
            BestDiff = TotalWeightedDiff;
            Result = AssetIndex;
        }
    }

    return(Result);
}


internal uint32
GetFirstAssetFrom(game_assets *Assets, asset_type_id TypeID)
{
    uint32 Result = 0;

    asset_type *Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
        Result = Type->FirstAssetIndex;
    }
    
    return(Result);
}

inline bitmap_id
GetFirstBitmapFrom(game_assets *Assets, asset_type_id TypeID)
{
    bitmap_id Result = {};
    Result.Value = GetFirstAssetFrom(Assets, TypeID);
    
    return(Result);
}

inline bitmap_id
GetBestMatchBitmapFrom(game_assets *Assets, asset_type_id TypeID,
                       asset_vector *MatchVector, asset_vector *WeightVector)
{
    bitmap_id Result = {};
    Result.Value = GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector);

    return(Result);
}

inline sound_id
GetFirstSoundFrom(game_assets *Assets, asset_type_id TypeID)
{
    sound_id Result = {};
    Result.Value = GetFirstAssetFrom(Assets, TypeID);
    
    return(Result);
}

inline sound_id
GetBestMatchSoundFrom(game_assets *Assets, asset_type_id TypeID,
                       asset_vector *MatchVector, asset_vector *WeightVector)
{
    sound_id Result = {};
    Result.Value = GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector);

    return(Result);
}

inline font_id
GetBestMatchFontFrom(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
{
    font_id Result = {};
    Result.Value = GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector);

    return(Result);
}

inline tileset_id
GetBestMatchTilesetFrom(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
{
    tileset_id Result = {};
    Result.Value = GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector);

    return(Result);
}

internal uint32
GetRandomAssetFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
{
    uint32 Result = 0;

    asset_type *Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
        uint32 Count = (Type->OnePastLastAssetIndex - Type->FirstAssetIndex);
        uint32 Choice = RandomChoice(Series, Count);
        Result = Type->FirstAssetIndex + Choice;
    }
    
    return(Result);
}

inline bitmap_id
GetRandomBitmapFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
{
    bitmap_id Result = {};
    Result.Value = GetRandomAssetFrom(Assets, TypeID, Series);

    return(Result);
}

internal game_assets *
AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
{
    game_assets *Assets = PushStruct(Arena, game_assets);

    Assets->NextGenerationID = 0;
    Assets->InFlightGenerationCount = 0;
    
    Assets->MemorySentinel.Flags = 0;
    Assets->MemorySentinel.Size = 0;
    Assets->MemorySentinel.Prev = &Assets->MemorySentinel;
    Assets->MemorySentinel.Next = &Assets->MemorySentinel;

    InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size));
    
    Assets->TranState = TranState;

    Assets->LoadedAssetSentinel.Next =
        Assets->LoadedAssetSentinel.Prev =
        &Assets->LoadedAssetSentinel;
    
    for(uint32 TagType = 0;
        TagType < Tag_Count;
        ++TagType)
    {
        Assets->TagRange[TagType] = 1000000.0f;
    }
    Assets->TagRange[Tag_FacingDirection] = 2.0f*Pi32;

    Assets->TagCount = 1;
    Assets->AssetCount = 1;

    {    
        platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin(PlatformFileType_AssetFile);
        Assets->FileCount = FileGroup.FileCount;
        Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
        for(u32 FileIndex = 0;
            FileIndex < Assets->FileCount;
            ++FileIndex)
        {
            asset_file *File = Assets->Files + FileIndex; 

            File->FontBitmapIDOffset = 0;
            File->TagBase = Assets->TagCount;
            
            ZeroStruct(File->Header);
            File->Handle = Platform.OpenNextFile(&FileGroup);
            Platform.ReadDataFromFile(&File->Handle, 0, sizeof(File->Header), &File->Header);

            u32 AssetTypeArraySize = File->Header.AssetTypeCount*sizeof(ssa_asset_type);
            File->AssetTypeArray = (ssa_asset_type *)PushSize(Arena, AssetTypeArraySize);
            Platform.ReadDataFromFile(&File->Handle, File->Header.AssetTypes,
                                     AssetTypeArraySize, File->AssetTypeArray);

            if(File->Header.MagicValue != SSA_MAGIC_VALUE)
            {
                Platform.FileError(&File->Handle, "SSA file has an invalid magic value.");
            }

            if(File->Header.Version > SSA_VERSION)
            {
                Platform.FileError(&File->Handle, "SSA file is of a later version.");
            }

            if(PlatformNoFileErrors(&File->Handle))
            {

                // NOTE(casey): The first asset and tag slot in every SSA is a null asset (reserved)
                // so we don't count it as something we will need space for!
                Assets->TagCount += (File->Header.TagCount - 1);
                Assets->AssetCount += (File->Header.AssetCount - 1);
            }
            else
            {
                // TODO(casey): Eventually, have some way of notifying users of bogus files?
                InvalidCodePath;
            }
        }

        Platform.GetAllFilesOfTypeEnd(&FileGroup);
    }        

    // NOTE(casey): Allocate all metadata space
    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
    Assets->Tags = PushArray(Arena, Assets->TagCount, ssa_tag);

    // NOTE(casey): Reserve one null tag at the beginning
    ZeroStruct(Assets->Tags[0]);

    // NOTE(casey): Load tags
    for(u32 FileIndex = 0;
        FileIndex < Assets->FileCount;
        ++FileIndex)
    {
        asset_file *File = Assets->Files + FileIndex; 
        if(PlatformNoFileErrors(&File->Handle))
        {
            // NOTE(casey): Skip the first tag
            u32 TagArraySize = sizeof(ssa_tag)*(File->Header.TagCount - 1);
            Platform.ReadDataFromFile(&File->Handle, File->Header.Tags + sizeof(ssa_tag),
                                     TagArraySize, Assets->Tags + File->TagBase);
        }
    }

    // NOTE(casey): Reserve one null asset at the beginning
    u32 AssetCount = 0;
    ZeroStruct(*(Assets->Assets + AssetCount));
    ++AssetCount;

    for(u32 DestTypeID = 0;
        DestTypeID < Asset_Count;
        ++DestTypeID)
    {
        asset_type *DestType = Assets->AssetTypes + DestTypeID;
        DestType->FirstAssetIndex = AssetCount;

        for(u32 FileIndex = 0;
            FileIndex < Assets->FileCount;
            ++FileIndex)
        {
            asset_file *File = Assets->Files + FileIndex; 
            if(PlatformNoFileErrors(&File->Handle))
            {
                for(u32 SourceIndex = 0;
                    SourceIndex < File->Header.AssetTypeCount;
                    ++SourceIndex)
                {
                    ssa_asset_type *SourceType = File->AssetTypeArray + SourceIndex;
                    if(SourceType->TypeID == DestTypeID)
                    {
                        if(SourceType->TypeID == Asset_FontGlyph)
                        {
                            File->FontBitmapIDOffset = AssetCount - SourceType->FirstAssetIndex;
                        }

                        if(SourceType->TypeID == Asset_Tile)
                        {
                            File->TileBitmapIDOffset = AssetCount - SourceType->FirstAssetIndex;
                        }
                        
                        u32 AssetCountForType = (SourceType->OnePastLastAssetIndex -
                                                 SourceType->FirstAssetIndex);

                        temporary_memory TempMem = BeginTemporaryMemory(&TranState->TranArena);
                        ssa_asset *SSAAssetArray = PushArray(&TranState->TranArena,
                                                             AssetCountForType, ssa_asset);
                        Platform.ReadDataFromFile(&File->Handle,
                                                 File->Header.Assets +
                                                 SourceType->FirstAssetIndex*sizeof(ssa_asset),
                                                 AssetCountForType*sizeof(ssa_asset),
                                                 SSAAssetArray);

                        for(u32 AssetIndex = 0;
                            AssetIndex < AssetCountForType;
                            ++AssetIndex)
                        {
                            ssa_asset *SSAAsset = SSAAssetArray + AssetIndex;

                            Assert(AssetCount < Assets->AssetCount);
                            asset *Asset = Assets->Assets + AssetCount++;
                            Asset->FileIndex = FileIndex;
                            Asset->SSA = *SSAAsset;
                            if(Asset->SSA.FirstTagIndex == 0)
                            {
                                Asset->SSA.FirstTagIndex = Asset->SSA.OnePastLastTagIndex = 0;
                            }
                            else
                            {
                                Asset->SSA.FirstTagIndex += (File->TagBase - 1);
                                Asset->SSA.OnePastLastTagIndex += (File->TagBase - 1);
                            }
                        }

                        EndTemporaryMemory(TempMem);
                    }
                }
            }
        }

        DestType->OnePastLastAssetIndex = AssetCount;
    }
    
    Assert(AssetCount == Assets->AssetCount);
    
    return(Assets);
}

inline u32
GetGlyphFromCodePoint(ssa_font *Info, loaded_font *Font, u32 CodePoint)
{
    u32 Result = 0;
    if(CodePoint < Info->OnePastHighestCodePoint)
    {
        Result = Font->UnicodeMap[CodePoint];
        Assert(Result < Info->GlyphCount);
    }

    return(Result);
}

internal real32
GetHorizontalAdvanceForPair(ssa_font *Info, loaded_font *Font, u32 DesiredPrevCodePoint, u32 DesiredCodePoint)
{
    u32 PrevGlyph = GetGlyphFromCodePoint(Info, Font, DesiredPrevCodePoint);
    u32 Glyph = GetGlyphFromCodePoint(Info, Font, DesiredCodePoint);

    r32 Result = Font->HorizontalAdvance[PrevGlyph*Info->GlyphCount + Glyph];
    
    return(Result);
}

internal bitmap_id
GetBitmapForGlyph(game_assets *Assets, ssa_font *Info, loaded_font *Font, u32 DesiredCodePoint)
{
    u32 Glyph = GetGlyphFromCodePoint(Info, Font, DesiredCodePoint);
    bitmap_id Result = Font->Glyphs[Glyph].BitmapID;
    Result.Value += Font->BitmapIDOffset;
    
    return(Result);
}

internal bitmap_id
GetBitmapForTileID(loaded_tileset *GlobalTileset, ssa_tileset *Info, u32 TileID, tileset_id ID)
{
    Assert(TileID < Info->TileCount);
    bitmap_id Result = GlobalTileset->Tiles[TileID].BitmapID;
    Result.Value += ID.Value;
    
    return(Result);
}

internal bitmap_id
GetBitmapForTile(game_assets *Assets, ssa_tileset *Info, loaded_tileset *Tileset,
                 u32 TileIndex)
{
    Assert(TileIndex < Info->TileCount);
    bitmap_id Result = Tileset->Tiles[TileIndex].BitmapID;
    Result.Value += Tileset->BitmapIDOffset;
    
    return(Result);
}

internal real32
GetLineAdvanceFor(ssa_font *Info)
{
    r32 Result = Info->AscenderHeight + Info->DescenderHeight + Info->ExternalLeading;

    return(Result);
}

internal real32
GetStartingBaselineY(ssa_font *Info)
{
    r32 Result = Info->AscenderHeight;

    return(Result);
}


