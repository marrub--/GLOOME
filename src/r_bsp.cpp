// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//		BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>

#include "templates.h"

#include "doomdef.h"

#include "m_bbox.h"

#include "i_system.h"
#include "p_lnspec.h"
#include "p_setup.h"

#include "r_main.h"
#include "r_plane.h"
#include "r_draw.h"
#include "r_things.h"
#include "r_3dfloors.h"
#include "a_sharedglobal.h"
#include "g_level.h"
#include "p_effect.h"

// State.
#include "doomstat.h"
#include "r_state.h"
#include "r_bsp.h"
#include "r_segs.h"
#include "v_palette.h"
#include "r_sky.h"
#include "po_man.h"
#include "r_data/colormaps.h"

seg_t*			curline;
side_t* 		sidedef;
line_t* 		linedef;
sector_t*		frontsector;
sector_t*		backsector;

int				doorclosed;

bool			r_fakingunderwater;

fixed_t			rw_backcz1, rw_backcz2;
fixed_t			rw_backfz1, rw_backfz2;
fixed_t			rw_frontcz1, rw_frontcz2;
fixed_t			rw_frontfz1, rw_frontfz2;


size_t			MaxDrawSegs;
drawseg_t		*drawsegs;
drawseg_t*		firstdrawseg;
drawseg_t*		ds_p;

size_t			FirstInterestingDrawseg;
TArray<size_t>	InterestingDrawsegs;

FWallCoords		WallC;
FWallTmapVals	WallT;

int WindowLeft, WindowRight;
WORD MirrorFlags;
seg_t *ActiveWallMirror;
TArray<size_t> WallMirrors;

CVAR (Bool, r_drawflat, false, 0)

void R_ClearDrawSegs (void)
{
}

bool R_ClipWallSegment (int first, int last, bool solid)
{
	return false;
}

bool R_CheckClipWallSegment (int first, int last)
{
	return false;
}

void R_ClearClipSegs (short left, short right)
{
}

sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec,
					 int *floorlightlevel, int *ceilinglightlevel,
					 bool back)
{
	return NULL;
}

void R_AddLine (seg_t *line)
{
}

bool FWallCoords::Init(int x1, int y1, int x2, int y2, int too_close)
{
	return false;
}

void FWallTmapVals::InitFromWallCoords(const FWallCoords *wallc)
{
}

void FWallTmapVals::InitFromLine(int tx1, int ty1, int tx2, int ty2)
{
}

extern "C" const int checkcoord[12][4] =
{
	{3,0,2,1},
	{3,0,2,0},
	{3,1,2,0},
	{0},
	{2,0,2,1},
	{0,0,0,0},
	{3,1,3,0},
	{0},
	{2,0,3,1},
	{2,1,3,1},
	{2,1,3,0}
};

void R_FakeDrawLoop(subsector_t *sub)
{
}

void R_Subsector (subsector_t *sub)
{
}

void R_RenderBSPNode (void *node)
{
}
