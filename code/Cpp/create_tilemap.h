#if !defined(CREATE_TILEMAP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
/*
  NOTE(casey):

  INVOKER_INTERNAL:
    0 - Build for public release
    1 - Build for developer only

  INVOKER_SLOW:
    0 - Not slow code allowed!
    1 - Slow code welcome.
*/

#if INVOKER_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif


#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline u32
SafeTruncateUint64(u64 Value)
{
    // TODO(casey): Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return(Result);
    
}

#define TILESIDE 30

struct game_offscreen_buffer
{
    // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

typedef struct debug_read_file_result
{
    u32 ContentsSize;
    void *Contents;
} debug_read_file_result;

struct loaded_bitmap
{
    i32 Width;
    i32 Height;
    u32 *Pixels;
};

struct bit_scan_result
{
    b32 Found;
    u32 Index;
};

struct colors
{
    u32 *Colors;
    i32 ColorCount;
    i32 ColorsCounters[256];
};

struct tile
{
    u32 Color;
    u16 Index;
    u16 Z;
};

struct game_memory
{
    b32 IsInitialized;

    u64 PermanentStorageSize;
    void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

    u64 TransientStorageSize;
    void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

};


struct game_button_state
{
    int HalfTransitionCount;
    b32 EndedDown;
};

struct game_controller_input
{
    b32 IsConnected;
    
    union
    {
        game_button_state Buttons[12];
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

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Back;
            game_button_state Start;

            // NOTE(casey): All buttons must be added above this line
            
            game_button_state Terminator;
        };
    };
};

struct game_input
{
    // TODO(casey): Insert clock values here.
    game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

internal void GameUpdateAndRender(game_memory *Memory);

//
struct game_state
{
    int ToneHz;
    int GreenOffset;
    int BlueOffset;
};


#define CREATE_TILEMAP_H
#endif
