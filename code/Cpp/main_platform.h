#if !defined(MAIN_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
    
typedef float real32;
typedef double real64;

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

struct bit_scan_result
{
    bool32 Found;
    uint32 Index;
};

struct debug_read_file_result
{
    uint32 ContentsSize;
    void *Contents;
};

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
};

uint32
SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return(Result);
}

#define MAIN_PLATFORM_H
#endif
