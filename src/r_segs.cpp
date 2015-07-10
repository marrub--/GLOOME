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
//		All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stddef.h>

#include "templates.h"
#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"
#include "doomdata.h"
#include "p_lnspec.h"

#include "r_local.h"
#include "r_sky.h"
#include "v_video.h"

#include "m_swap.h"
#include "w_wad.h"
#include "stats.h"
#include "a_sharedglobal.h"
#include "d_net.h"
#include "g_level.h"
#include "r_bsp.h"
#include "r_plane.h"
#include "r_segs.h"
#include "r_3dfloors.h"
#include "v_palette.h"
#include "r_data/colormaps.h"

#define WALLYREPEAT 8


CVAR(Bool, r_np2, true, 0)

#define HEIGHTBITS 12
#define HEIGHTSHIFT (FRACBITS-HEIGHTBITS)

extern fixed_t globaluclip, globaldclip;

bool		markfloor;		// False if the back side is the same plane.
bool		markceiling;
FTexture *toptexture;
FTexture *bottomtexture;
FTexture *midtexture;
fixed_t rw_offset_top;
fixed_t rw_offset_mid;
fixed_t rw_offset_bottom;


int		wallshade;

short	walltop[MAXWIDTH];	// [RH] record max extents of wall
short	wallbottom[MAXWIDTH];
short	wallupper[MAXWIDTH];
short	walllower[MAXWIDTH];
fixed_t	swall[MAXWIDTH];
fixed_t	lwall[MAXWIDTH];
fixed_t	lwallscale;

int				rw_ceilstat, rw_floorstat;
bool			rw_mustmarkfloor, rw_mustmarkceiling;
bool			rw_prepped;
bool			rw_markmirror;
bool			rw_havehigh;
bool			rw_havelow;

fixed_t			rw_light;		// [RH] Scale lights with viewsize adjustments
fixed_t			rw_lightstep;
fixed_t			rw_lightleft;

fixed_t			rw_offset;

FTexture		*rw_pic;

void wallscan_np2(int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat, fixed_t top, fixed_t bot, bool mask);

//=============================================================================
//
// CVAR r_fogboundary
//
// If true, makes fog look more "real" by shading the walls separating two
// sectors with different fog.
//=============================================================================

CVAR(Bool, r_fogboundary, true, 0)

inline bool IsFogBoundary (sector_t *front, sector_t *back)
{
}

CVAR(Bool, r_drawmirrors, true, 0)

fixed_t *MaskedSWall;
fixed_t MaskedScaleY;

void ClipMidtex(int x1, int x2)
{
}

void R_RenderFakeWallRange(drawseg_t *ds, int x1, int x2);

void R_RenderMaskedSegRange (drawseg_t *ds, int x1, int x2)
{
}

void R_RenderFakeWall(drawseg_t *ds, int x1, int x2, F3DFloor *rover)
{
}

void R_RenderFakeWallRange (drawseg_t *ds, int x1, int x2)
{
}

inline fixed_t prevline1 (fixed_t vince, BYTE *colormap, int count, fixed_t vplce, const BYTE *bufplce, BYTE *dest)
{
	return 0;
}

void wallscan (int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal,
			   fixed_t yrepeat, const BYTE *(*getcol)(FTexture *tex, int x))
{
}

void wallscan_striped (int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat)
{
}

void wallscan_np2(int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat, fixed_t top, fixed_t bot, bool mask)
{
}

inline fixed_t mvline1 (fixed_t vince, BYTE *colormap, int count, fixed_t vplce, const BYTE *bufplce, BYTE *dest)
{
	return 0;
}

void maskwallscan (int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal,
	fixed_t yrepeat, const BYTE *(*getcol)(FTexture *tex, int x))
{
}

inline void preptmvline1 (fixed_t vince, BYTE *colormap, int count, fixed_t vplce, const BYTE *bufplce, BYTE *dest)
{
}

void transmaskwallscan (int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal,
	fixed_t yrepeat, const BYTE *(*getcol)(FTexture *tex, int x))
{
}

void R_RenderSegLoop ()
{
}

void R_NewWall (bool needlights)
{
}

void R_CheckDrawSegs ()
{
}

ptrdiff_t R_NewOpening (ptrdiff_t len)
{
	return 0;
}

void R_StoreWallRange (int start, int stop)
{
}

int OWallMost (short *mostbuf, fixed_t z, const FWallCoords *wallc)
{
	return 0;
}

int WallMost (short *mostbuf, const secplane_t &plane, const FWallCoords *wallc)
{
	return 0;
}

void PrepWall (fixed_t *swall, fixed_t *lwall, fixed_t walxrepeat, int x1, int x2)
{
}

void PrepLWall (fixed_t *lwall, fixed_t walxrepeat, int x1, int x2)
{
}
