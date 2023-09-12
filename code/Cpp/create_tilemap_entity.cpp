/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Handy Paul $
   $Notice: (C) Copyright 2023 by Handy Paul, Inc. All Rights Reserved. $
   ======================================================================== */

inline move_spec
DefaultMoveSpec(void)
{
    move_spec Result;
    Result.UnitMaxAccelVector = false;
    Result.Speed = 1.0f;
    Result.Drag = 0.0f;

    return(Result);
}
