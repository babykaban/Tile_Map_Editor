#if !defined(EDITOR_COLLISION_MODE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct collision
{
    world_position P;
    rectangle2 Rect;
};

struct edit_mode_collision
{
    collision *Collisions;
};

#define EDITOR_COLLISION_MODE_H
#endif
