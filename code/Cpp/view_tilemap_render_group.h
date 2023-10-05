#if !defined(VIEW_TILEMAP_RENDER_GROUP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

/* NOTE(casey):

   1) Everywhere outside the renderer, Y -always_ goes upward, X to the right.

   2) All bitmaps including the render target are asummed to be bottom-up
      (meaning that the first row pointer points to the bottom-most row
       when viewed on screen).

   3) It is mandatory that all inputs to the renderer are in world
      coordinate ("meters"), NOT pixels. If for some reason something
      absolutely has to be specefied in pixelsm that will be explicitly
      marked in the API, but this should occur exceedingly sparingly.

   4) Z is a special coordinate because it is broken up into discrete slices,
      and the renderer actually understands these slices. Z slices are what
      control the _scaling_ of things whereas Z offsets inside a slice are
      what control Y offsetting.

      // TODO(casey): ZHANDLING
      
   5) All color values specified to the renderer as V4's are in
      NON-premultiplied alpha.

*/

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 SizeOfBitmap;
    int32 HorzResolution;
    int32 VertResolution;
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
    uint32 AlphaMask;
};
#pragma pack(pop)

struct loaded_bitmap
{
    v2 AlignPercentage;
    real32 WidthOverHeight;
    
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
};

struct environment_map
{
    loaded_bitmap LOD[4];
    real32 Pz;
};

struct render_basis
{
    v3 P;
};

enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_clear,
    RenderGroupEntryType_render_entry_bitmap,
    RenderGroupEntryType_render_entry_rectangle,
    RenderGroupEntryType_render_entry_coordinate_system,
};

struct render_group_entry_header
{
    render_group_entry_type Type;
};

struct render_entry_clear
{
    v4 Color;
};

struct render_entry_saturation
{
    real32 Level;
};

struct render_entry_bitmap
{
    loaded_bitmap *Bitmap;

    v4 Color;
    v2 P;
    v2 Size;
};

struct render_entry_rectangle
{
    v4 Color;
    v2 P;
    v2 Dim;
};

// NOTE(casey): This is only for test:
// {
struct render_entry_coordinate_system
{
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    v4 Color;
    loaded_bitmap *Texture;
    loaded_bitmap *NormalMap;

    //real32 MetersToPixels;

    environment_map *Top;
    environment_map *Middle;
    environment_map *Bottom;
};
// }

struct render_transform
{
    bool32 Ortographic;

    // NOTE(casey): Camera parameters
    real32 MetersToPixels; // NOTE(casey): This is translates meters _on the monitor_ into pixels _on the monitor_
    v2 ScreenCenter;
    
    real32 FocalLength;
    real32 DistanceAboveTarget;

    v3 OffsetP;
    real32 Scale;
};

struct render_group
{
    real32 GlobalAlpha;

    v2 MonitorHalfDimInMeters;
    render_transform Transform;

    uint32 MaxPushBufferSize;
    uint32 PushBufferSize;
    uint8 *PushBufferBase;
};

#define VIEW_TILEMAP_RENDER_GROUP_H
#endif
