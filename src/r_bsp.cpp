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

// killough 4/7/98: indicates doors closed wrt automap bugfix:
int				doorclosed;

bool			r_fakingunderwater;

extern bool		rw_prepped;
extern bool		rw_havehigh, rw_havelow;
extern int		rw_floorstat, rw_ceilstat;
extern bool		rw_mustmarkfloor, rw_mustmarkceiling;
extern short	walltop[MAXWIDTH];	// [RH] record max extents of wall
extern short	wallbottom[MAXWIDTH];
extern short	wallupper[MAXWIDTH];
extern short	walllower[MAXWIDTH];

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

static BYTE		FakeSide;

int WindowLeft, WindowRight;
WORD MirrorFlags;
seg_t *ActiveWallMirror;
TArray<size_t> WallMirrors;

static subsector_t *InSubsector;

CVAR (Bool, r_drawflat, false, 0)		// [RH] Don't texture segs?


void R_StoreWallRange (int start, int stop);

//
// R_ClearDrawSegs
//
void R_ClearDrawSegs (void)
{
}



//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
//
// 1/11/98 killough: Since a type "short" is sufficient, we
// should use it, since smaller arrays fit better in cache.
//

struct cliprange_t
{
	short first, last;		// killough
};


// newend is one past the last valid seg
static cliprange_t     *newend;
static cliprange_t		solidsegs[MAXWIDTH/2+2];



//==========================================================================
//
// R_ClipWallSegment
//
// Clips the given range of columns, possibly including it in the clip list.
// Handles both windows (e.g. LineDefs with upper and lower textures) and
// solid walls (e.g. single sided LineDefs [middle texture]) that entirely
// block the view.
//
//==========================================================================

bool R_ClipWallSegment (int first, int last, bool solid)
{
	return false;
}

bool R_CheckClipWallSegment (int first, int last)
{
	return false;
}



//
// R_ClearClipSegs
//
void R_ClearClipSegs (short left, short right)
{
}


//
// killough 3/7/98: Hack floor/ceiling heights for deep water etc.
//
// If player's view height is underneath fake floor, lower the
// drawn ceiling to be just under the floor height, and replace
// the drawn floor and ceiling textures, and light level, with
// the control sector's.
//
// Similar for ceiling, only reflected.
//
// killough 4/11/98, 4/13/98: fix bugs, add 'back' parameter
//

sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec,
					 int *floorlightlevel, int *ceilinglightlevel,
					 bool back)
{
	return NULL;
}


//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//

void R_AddLine (seg_t *line)
{
}

//
// FWallCoords :: Init
//
// Transform and clip coordinates. Returns true if it was clipped away
//
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

//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
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


static bool R_CheckBBox (fixed_t *bspcoord)	// killough 1/28/98: static
{
	return false;
}


void R_Subsector (subsector_t *sub);
static void R_AddPolyobjs(subsector_t *sub)
{
}

// kg3D - add fake segs, never rendered
void R_FakeDrawLoop(subsector_t *sub)
{
}

//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//
void R_Subsector (subsector_t *sub)
{
}

//
// RenderBSPNode
// Renders all subsectors below a given node, traversing subtree recursively.
// Just call with BSP root and -1.
// killough 5/2/98: reformatted, removed tail recursion

void R_RenderBSPNode (void *node)
{
}
