#if !defined(EDITOR_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   ======================================================================== */

/*
  NOTE(casey):

  EDITOR_INTERNAL:
    0 - Build for public release
    1 - Build for developer only

  EDITOR_SLOW:
    0 - Not slow code allowed!
    1 - Slow code welcome.
*/

#ifdef __cplusplus
extern "C" {
#endif

//
// NOTE(casey): Compilers
//

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
// TODO(casey): Moar compilerz!!!
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#include <stdio.h>
#endif
    
//
// NOTE(casey): Types
//
    
#include <stdint.h>
#include <stddef.h>    
#include <limits.h>
#include <float.h>
    
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;
    
typedef size_t memory_index;
    
typedef float real32;
typedef double real64;
    
typedef int8 s8;
typedef int8 s08;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;
typedef bool32 b32;

typedef uint8 u8;
typedef uint8 u08;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef real32 r32;
typedef real64 r64;

typedef real32 r32;
typedef real64 r64;

typedef uintptr_t umm;

#define U32FromPointer(Pointer) ((u32)(memory_index)(Pointer))
#define PointerFromU32(type, Value) (type *)((memory_index)Value)

union v2
{
    struct
    {
        real32 x, y;
    };
    struct
    {
        real32 u, v;
    };
    real32 E[2];
};

union v3
{
    struct
    {
        real32 x, y, z;
    };
    struct
    {
        real32 u, v, w;
    };
    struct
    {
        real32 r, g, b;
    };
    struct
    {
        v2 xy;
        real32 Ignored0_;
    };
    struct
    {
        real32 Ignored1_;
        v2 yz;
    };
    struct
    {
        v2 uv;
        real32 Ignored2_;
    };
    struct
    {
        real32 Ignored3_;
        v2 vw;
    };
    real32 E[3];
};

union v4
{
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                real32 x, y, z;
            };
        };
        
        real32 w;        
    };
    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                real32 r, g, b;
            };
        };
        
        real32 a;        
    };
    struct
    {
        v2 xy;
        real32 Ignored0_;
        real32 Ignored1_;
    };
    struct
    {
        real32 Ignored2_;
        v2 yz;
        real32 Ignored3_;
    };
    struct
    {
        real32 Ignored4_;
        real32 Ignored5_;
        v2 zw;
    };
    real32 E[4];
};

struct rectangle2
{
    v2 Min;
    v2 Max;
};

struct rectangle3
{
    v3 Min;
    v3 Max;
};

    
#define Real32Maximum FLT_MAX
#define Real32Minimum -FLT_MAX
    
#define internal static 
#define local_persist static 
#define global_variable static

#define Pi32 3.14159265359f

#if EDITOR_SLOW
// TODO(casey): Complete assertion macro - don't worry everyone!
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break
    
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
// TODO(casey): swap, min, max ... macros???

#define AlignPow2(Value, Alignment) ((Value + (((Alignment) - 1) & ~((Alignment) - 1))))
#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)

inline u16
SafeTruncateToU16(uint32 Value)
{
    // TODO(casey): Defines for maximum values
    Assert(Value <= 0xFFFF);
    u16 Result = (u16)Value;
    return(Result);
}
    
inline uint32
SafeTruncateUInt64(uint64 Value)
{
    // TODO(casey): Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return(Result);
}
    
inline uint16
SafeTruncateToUInt16(uint32 Value)
{
    // TODO(casey): Defines for maximum values
    Assert(Value <= 65535);
    Assert(Value >= 0);
    uint16 Result = (uint16)Value;
    return(Result);
}
    
inline int16
SafeTruncateToInt16(int32 Value)
{
    // TODO(casey): Defines for maximum values
    Assert(Value < 32767);
    Assert(Value >= -32768);
    int16 Result = (int16)Value;
    return(Result);
}

inline void *
MemoryCopy(void *Dest, void *Source, uint32 Size)
{
    uint8 *Dst = (uint8 *)Dest;
    uint8 *Src = (uint8 *)Source;

    for(uint32 MemoryIndex = 0;
        MemoryIndex < Size;
        ++MemoryIndex)
    {
        Dst[MemoryIndex] = Src[MemoryIndex];
    }

    return(Dest);
}

/*
  NOTE(casey): Services that the platform layer provides to the game
*/
#if EDITOR_INTERNAL
/* IMPORTANT(casey):

   These are NOT for doing anything in the shipping game - they are
   blocking and the write doesn't protect against lost data!
*/
typedef struct read_file_result
{
    uint32 ContentsSize;
    void *Contents;
} read_file_result;

#define PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

#define PLATFORM_READ_ENTIRE_FILE_A(name) read_file_result name(char *Filename)
typedef PLATFORM_READ_ENTIRE_FILE_A(platform_read_entire_file);

#define PLATFORM_WRITE_ENTIRE_FILE_A(name) bool32 name(char *Filename, uint32 MemorySize, void *Memory)
typedef PLATFORM_WRITE_ENTIRE_FILE_A(platform_write_entire_file);
    
#endif

/*
  NOTE(casey): Services that the game provides to the platform layer.
  (this may expand in the future - sound on separate thread, etc.)
*/

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

// TODO(casey): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
#define BITMAP_BYTES_PER_PIXEL 4
typedef struct game_offscreen_buffer
{
    // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
} game_offscreen_buffer;

typedef struct game_render_commands
{
    u32 Width;
    u32 Height;
    
    u32 MaxPushBufferSize;
    u32 PushBufferSize;
    u8 *PushBufferBase;
    
    u32 PushBufferElementCount;
    u32 SortEntryAt;
    
    u32 ClipRectCount;
    struct render_entry_cliprect *ClipRects;
    
    render_entry_cliprect *FirstRect;
    render_entry_cliprect *LastRect;
} game_render_commands;

#define RenderCommandStruct(MaxPushBufferSize, PushBuffer, Width, Height) \
    {Width, Height, MaxPushBufferSize, 0, (u8 *)PushBuffer, 0, MaxPushBufferSize};

typedef struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
} game_sound_output_buffer;

typedef struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
    bool32 Reset;
} game_button_state;

typedef struct game_controller_input
{
    bool32 IsConnected;
    bool32 IsAnalog;    
    real32 StickAverageX;
    real32 StickAverageY;
    
    union
    {
        game_button_state Buttons[19];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;
            
            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state Biome;
            game_button_state Type;
            game_button_state Height;
            game_button_state CliffHillType;
            game_button_state MainSurface;
            game_button_state MergeSurface;

            game_button_state ChangeEditMode;
            
            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Back;
            game_button_state Start;

            // NOTE(casey): All buttons must be added above this line
            
            game_button_state Terminator;
        };
    };
} game_controller_input;

typedef struct game_input
{
    game_button_state MouseButtons[5];
    int32 MouseX, MouseY;
    int16 MouseZ;
    
    real32 ExecutableReloaded;
    real32 dtForFrame;

    game_controller_input Controllers[5];
} game_input;

inline b32 WasPressed(game_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));

    return(Result);
}

typedef struct platform_file_handle
{
    b32 NoErrors;
    void *Platform;
} platform_file_handle;

typedef struct platform_file_group
{
    u32 FileCount;
    void *Platform;
} platform_file_group;

typedef enum platform_file_type
{
    PlatformFileType_AssetFile,
    PlatformFileType_SavedGameFile,
    PlatformFileType_BMP,
    PlatformFileType_BAF,

    PlatformFileType_Count,
} platform_file_type;

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_W(name) platform_file_group name(platform_file_type Type, wchar_t *Path)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_W(platform_get_all_files_of_type_begin_w);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_A(name) platform_file_group name(platform_file_type Type, char *Path)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_A(platform_get_all_files_of_type_begin_a);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END_W(name) void name(platform_file_group *FileGroup)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END_W(platform_get_all_files_of_type_end_w);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END_A(name) void name(platform_file_group *FileGroup)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END_A(platform_get_all_files_of_type_end_a);

#define PLATFORM_OPEN_NEXT_FILE_W(name) platform_file_handle name(platform_file_group *FileGroup)
typedef PLATFORM_OPEN_NEXT_FILE_W(platform_open_next_file_w);

#define PLATFORM_OPEN_NEXT_FILE_A(name) platform_file_handle name(platform_file_group *FileGroup)
typedef PLATFORM_OPEN_NEXT_FILE_A(platform_open_next_file_a);

#define PLATFORM_GET_NEXT_FILE_NAME_BEGIN_W(name) wchar_t *name(platform_file_group *FileGroup)
typedef PLATFORM_GET_NEXT_FILE_NAME_BEGIN_W(platform_get_next_file_name_begin_w);

#define PLATFORM_GET_NEXT_FILE_NAME_END_W(name) void name(platform_file_group *FileGroup)
typedef PLATFORM_GET_NEXT_FILE_NAME_END_W(platform_get_next_file_name_end_w);

#define PLATFORM_GET_NEXT_FILE_NAME_BEGIN_A(name) char *name(platform_file_group *FileGroup)
typedef PLATFORM_GET_NEXT_FILE_NAME_BEGIN_A(platform_get_next_file_name_begin_a);

#define PLATFORM_GET_NEXT_FILE_NAME_END_A(name) void name(platform_file_group *FileGroup)
typedef PLATFORM_GET_NEXT_FILE_NAME_END_A(platform_get_next_file_name_end_a);

#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle *Source, u64 Offset, u64 Size, void *Dest)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle *Handle, char *Message)
typedef PLATFORM_FILE_ERROR(platform_file_error);

#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *Queue, void *Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

#define PLATFORM_ALLOCATE_TEXTURE(name) void *name(u32 Width, u32 Height, void *Data)
typedef PLATFORM_ALLOCATE_TEXTURE(platform_allocate_texture);

#define PLATFORM_DEALLOCATE_TEXTURE(name) void name(void *Texture)
typedef PLATFORM_DEALLOCATE_TEXTURE(platform_deallocate_texture);

#define PLATFORM_ALLOCATE_MEMORY(name) void *name(memory_index Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void *Memory)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

typedef void platform_add_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

typedef struct platform_api
{
    platform_add_entry *AddEntry;
    platform_complete_all_work *CompleteAllWork;
    
    platform_allocate_texture *AllocateTexture;
    platform_deallocate_texture *DeallocateTexture;

    platform_get_all_files_of_type_begin_w *GetAllFilesOfTypeBeginW;
    platform_get_all_files_of_type_begin_a *GetAllFilesOfTypeBeginA;
    platform_get_all_files_of_type_end_w *GetAllFilesOfTypeEndW;
    platform_get_all_files_of_type_end_a *GetAllFilesOfTypeEndA;
    platform_open_next_file_w *OpenNextFileW;
    platform_open_next_file_a *OpenNextFileA;
    platform_get_next_file_name_begin_w *GetNextFileNameBeginW;
    platform_get_next_file_name_end_w *GetNextFileNameEndW;
    platform_get_next_file_name_begin_a *GetNextFileNameBeginA;
    platform_get_next_file_name_end_a *GetNextFileNameEndA;
    platform_read_data_from_file *ReadDataFromFile;
    platform_file_error *FileError;

    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;
    
    platform_free_file_memory *FreeFileMemory;
    platform_read_entire_file *ReadEntireFileA;
    platform_write_entire_file *WriteEntireFileA;

} platform_api;

typedef struct game_memory 
{
    bool32 IsInitialized;

    uint64 PermanentStorageSize;
    void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

    uint64 TransientStorageSize;
    void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;

    platform_api PlatformAPI;
} game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_render_commands *RenderCommands)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE(casey): At the moment, this has to be a very fast function, it cannot be
// more than a millisecond or so.

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

#define CompletePreviousReadsBeforeFutureReads _ReadBarrier();
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier();

inline uint32
AtomicCompareExchangeUInt32(uint32 volatile *Value, uint32 New, uint32 Expected)
{
    uint32 Result = _InterlockedCompareExchange((long *)Value, New, Expected);

    return(Result);
}

inline uint32
AtomicAddU32(uint32 volatile *Value, uint32 Addend)
{
    // NOTE(casey): Returns the original value _prior_ to adding
    uint32 Result = _InterlockedExchangeAdd((long *)Value, Addend);

    return(Result);
}

inline uint64
AtomicExchangeU64(uint64 volatile *Value, uint64 New)
{
    uint64 Result = _InterlockedExchange64((__int64 *)Value, New);

    return(Result);
}

inline uint64
AtomicAddU64(uint64 volatile *Value, uint64 Addend)
{
    // NOTE(casey): Returns the original value _prior_ to adding
    uint64 Result = _InterlockedExchangeAdd64((__int64 *)Value, Addend);

    return(Result);
}

inline u32
GetThreadID(void)
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);

    return(ThreadID);
}

#ifdef __cplusplus
}
#endif

#define EDITOR_PLATFORM_H
#endif
