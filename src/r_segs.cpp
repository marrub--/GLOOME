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

//CVAR (Int, ty, 8, 0)
//CVAR (Int, tx, 8, 0)

#define HEIGHTBITS 12
#define HEIGHTSHIFT (FRACBITS-HEIGHTBITS)

extern fixed_t globaluclip, globaldclip;


// OPTIMIZE: closed two sided lines as single sided

// killough 1/6/98: replaced globals with statics where appropriate

static bool		segtextured;	// True if any of the segs textures might be visible.
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

//
// regular wall
//
extern fixed_t	rw_backcz1, rw_backcz2;
extern fixed_t	rw_backfz1, rw_backfz2;
extern fixed_t	rw_frontcz1, rw_frontcz2;
extern fixed_t	rw_frontfz1, rw_frontfz2;

int				rw_ceilstat, rw_floorstat;
bool			rw_mustmarkfloor, rw_mustmarkceiling;
bool			rw_prepped;
bool			rw_markmirror;
bool			rw_havehigh;
bool			rw_havelow;

fixed_t			rw_light;		// [RH] Scale lights with viewsize adjustments
fixed_t			rw_lightstep;
fixed_t			rw_lightleft;

static fixed_t	rw_frontlowertop;

static int		rw_x;
static int		rw_stopx;
fixed_t			rw_offset;
static fixed_t	rw_scalestep;
static fixed_t	rw_midtexturemid;
static fixed_t	rw_toptexturemid;
static fixed_t	rw_bottomtexturemid;
static fixed_t	rw_midtexturescalex;
static fixed_t	rw_midtexturescaley;
static fixed_t	rw_toptexturescalex;
static fixed_t	rw_toptexturescaley;
static fixed_t	rw_bottomtexturescalex;
static fixed_t	rw_bottomtexturescaley;

FTexture		*rw_pic;

static fixed_t	*maskedtexturecol;

static void R_RenderDecal (side_t *wall, DBaseDecal *first, drawseg_t *clipper, int pass);
static void WallSpriteColumn (void (*drawfunc)(const BYTE *column, const FTexture::Span *spans));
void wallscan_np2(int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat, fixed_t top, fixed_t bot, bool mask);
static void wallscan_np2_ds(drawseg_t *ds, int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat);
static void call_wallscan(int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat, bool mask);

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
	return r_fogboundary && fixedcolormap == NULL && front->ColorMap->Fade &&
		front->ColorMap->Fade != back->ColorMap->Fade &&
		(front->GetTexture(sector_t::ceiling) != skyflatnum || back->GetTexture(sector_t::ceiling) != skyflatnum);
}

//=============================================================================
//
// CVAR r_drawmirrors
//
// Set to false to disable rendering of mirrors
//=============================================================================

CVAR(Bool, r_drawmirrors, true, 0)

//
// R_RenderMaskedSegRange
//
fixed_t *MaskedSWall;
fixed_t MaskedScaleY;

static void BlastMaskedColumn (void (*blastfunc)(const BYTE *pixels, const FTexture::Span *spans), FTexture *tex)
{
	if (maskedtexturecol[dc_x] != FIXED_MAX)
	{
		// calculate lighting
		if (fixedcolormap == NULL && fixedlightlev < 0)
		{
			dc_colormap = basecolormap->Maps + (GETPALOOKUP (rw_light, wallshade) << COLORMAPSHIFT);
		}

		dc_iscale = MulScale18 (MaskedSWall[dc_x], MaskedScaleY);
		sprtopscreen = centeryfrac - FixedMul (dc_texturemid, spryscale);
		
		// killough 1/25/98: here's where Medusa came in, because
		// it implicitly assumed that the column was all one patch.
		// Originally, Doom did not construct complete columns for
		// multipatched textures, so there were no header or trailer
		// bytes in the column referred to below, which explains
		// the Medusa effect. The fix is to construct true columns
		// when forming multipatched textures (see r_data.c).

		// draw the texture
		const FTexture::Span *spans;
		const BYTE *pixels = tex->GetColumn (maskedtexturecol[dc_x] >> FRACBITS, &spans);
		blastfunc (pixels, spans);
//		maskedtexturecol[dc_x] = FIXED_MAX; // kg3D - seems to be useless
	}
	rw_light += rw_lightstep;
	spryscale += rw_scalestep;
}

// Clip a midtexture to the floor and ceiling of the sector in front of it.
void ClipMidtex(int x1, int x2)
{
	short most[MAXWIDTH];

	WallMost(most, curline->frontsector->ceilingplane, &WallC);
	for (int i = x1; i <= x2; ++i)
	{
		if (wallupper[i] < most[i])
			wallupper[i] = most[i];
	}
	WallMost(most, curline->frontsector->floorplane, &WallC);
	for (int i = x1; i <= x2; ++i)
	{
		if (walllower[i] > most[i])
			walllower[i] = most[i];
	}
}

void R_RenderFakeWallRange(drawseg_t *ds, int x1, int x2);

void R_RenderMaskedSegRange (drawseg_t *ds, int x1, int x2)
{
	FTexture	*tex;
	int			i;
	sector_t	tempsec;		// killough 4/13/98
	fixed_t		texheight, textop, texheightscale;
	bool		notrelevant = false;

	const sector_t *sec;

	sprflipvert = false;

	curline = ds->curline;

	// killough 4/11/98: draw translucent 2s normal textures
	// [RH] modified because we don't use user-definable translucency maps
	ESPSResult drawmode;

	drawmode = R_SetPatchStyle (LegacyRenderStyles[curline->linedef->flags & ML_ADDTRANS ? STYLE_Add : STYLE_Translucent],
		MIN(curline->linedef->Alpha, FRACUNIT),	0, 0);

	if ((drawmode == DontDraw && !ds->bFogBoundary && !ds->bFakeBoundary))
	{
		return;
	}

	NetUpdate ();

	frontsector = curline->frontsector;
	backsector = curline->backsector;

	tex = TexMan(curline->sidedef->GetTexture(side_t::mid), true);
	if (i_compatflags & COMPATF_MASKEDMIDTEX)
	{
		tex = tex->GetRawTexture();
	}

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	sec = R_FakeFlat (frontsector, &tempsec, NULL, NULL, false);

	basecolormap = sec->ColorMap;	// [RH] Set basecolormap

	wallshade = ds->shade;
	rw_lightstep = ds->lightstep;
	rw_light = ds->light + (x1 - ds->x1) * rw_lightstep;

	if (fixedlightlev < 0)
	{
		for (i = frontsector->e->XFloor.lightlist.Size() - 1; i >= 0; i--)
		{
			if (!(fake3D & FAKE3D_CLIPTOP))
			{
				sclipTop = sec->ceilingplane.ZatPoint(viewx, viewy);
			}
			if (sclipTop <= frontsector->e->XFloor.lightlist[i].plane.ZatPoint(viewx, viewy))
			{
				lightlist_t *lit = &frontsector->e->XFloor.lightlist[i];
				basecolormap = lit->extra_colormap;
				wallshade = LIGHT2SHADE(curline->sidedef->GetLightLevel(foggy, *lit->p_lightlevel, lit->lightsource == NULL) + r_actualextralight);
				break;
			}
		}
	}

	mfloorclip = openings + ds->sprbottomclip - ds->x1;
	mceilingclip = openings + ds->sprtopclip - ds->x1;

	// [RH] Draw fog partition
	if (ds->bFogBoundary)
	{
		R_DrawFogBoundary (x1, x2, mceilingclip, mfloorclip);
		if (ds->maskedtexturecol == -1)
		{
			goto clearfog;
		}
	}
	if ((ds->bFakeBoundary && !(ds->bFakeBoundary & 4)) || drawmode == DontDraw)
	{
		goto clearfog;
	}

	MaskedSWall = (fixed_t *)(openings + ds->swall) - ds->x1;
	MaskedScaleY = ds->yrepeat;
	maskedtexturecol = (fixed_t *)(openings + ds->maskedtexturecol) - ds->x1;
	spryscale = ds->iscale + ds->iscalestep * (x1 - ds->x1);
	rw_scalestep = ds->iscalestep;

	// find positioning
	texheight = tex->GetScaledHeight() << FRACBITS;
	texheightscale = curline->sidedef->GetTextureYScale(side_t::mid);
	if (texheightscale != FRACUNIT)
	{
		texheight = FixedDiv(texheight, texheightscale);
	}
	if (curline->linedef->flags & ML_DONTPEGBOTTOM)
	{
		dc_texturemid = MAX (frontsector->GetPlaneTexZ(sector_t::floor), backsector->GetPlaneTexZ(sector_t::floor)) + texheight;
	}
	else
	{
		dc_texturemid = MIN (frontsector->GetPlaneTexZ(sector_t::ceiling), backsector->GetPlaneTexZ(sector_t::ceiling));
	}

	{ // encapsulate the lifetime of rowoffset
		fixed_t rowoffset = curline->sidedef->GetTextureYOffset(side_t::mid);
		if (tex->bWorldPanning)
		{
			// rowoffset is added before the MulScale3 so that the masked texture will
			// still be positioned in world units rather than texels.
			dc_texturemid += rowoffset - viewz;
			textop = dc_texturemid;
			dc_texturemid = MulScale16 (dc_texturemid, MaskedScaleY);
		}
		else
		{
			// rowoffset is added outside the multiply so that it positions the texture
			// by texels instead of world units.
			textop = dc_texturemid - viewz + SafeDivScale16 (rowoffset, MaskedScaleY);
			dc_texturemid = MulScale16 (dc_texturemid - viewz, MaskedScaleY) + rowoffset;
		}
	}

	if (fixedlightlev >= 0)
		dc_colormap = basecolormap->Maps + fixedlightlev;
	else if (fixedcolormap != NULL)
		dc_colormap = fixedcolormap;

	if (!(curline->linedef->flags & ML_WRAP_MIDTEX) &&
		!(curline->sidedef->Flags & WALLF_WRAP_MIDTEX))
	{ // Texture does not wrap vertically.

		// [RH] Don't bother drawing segs that are completely offscreen
		if (MulScale12 (globaldclip, ds->sz1) < -textop &&
			MulScale12 (globaldclip, ds->sz2) < -textop)
		{ // Texture top is below the bottom of the screen
			goto clearfog;
		}

		if (MulScale12 (globaluclip, ds->sz1) > texheight - textop &&
			MulScale12 (globaluclip, ds->sz2) > texheight - textop)
		{ // Texture bottom is above the top of the screen
			goto clearfog;
		}

		if ((fake3D & FAKE3D_CLIPBOTTOM) && textop < sclipBottom - viewz)
		{
			notrelevant = true;
			goto clearfog;
		}
		if ((fake3D & FAKE3D_CLIPTOP) && textop - texheight > sclipTop - viewz)
		{
			notrelevant = true;
			goto clearfog;
		}

		WallC.sz1 = ds->sz1;
		WallC.sz2 = ds->sz2;
		WallC.sx1 = ds->sx1;
		WallC.sx2 = ds->sx2;

		if (fake3D & FAKE3D_CLIPTOP)
		{
			OWallMost(wallupper, textop < sclipTop - viewz ? textop : sclipTop - viewz, &WallC);
		}
		else
		{
			OWallMost(wallupper, textop, &WallC);
		}
		if (fake3D & FAKE3D_CLIPBOTTOM)
		{
			OWallMost(walllower, textop - texheight > sclipBottom - viewz ? textop - texheight : sclipBottom - viewz, &WallC);
		}
		else
		{
			OWallMost(walllower, textop - texheight, &WallC);
		}

		for (i = x1; i <= x2; i++)
		{
			if (wallupper[i] < mceilingclip[i])
				wallupper[i] = mceilingclip[i];
		}
		for (i = x1; i <= x2; i++)
		{
			if (walllower[i] > mfloorclip[i])
				walllower[i] = mfloorclip[i];
		}

		if (CurrentSkybox)
		{ // Midtex clipping doesn't work properly with skyboxes, since you're normally below the floor
		  // or above the ceiling, so the appropriate end won't be clipped automatically when adding
		  // this drawseg.
			if ((curline->linedef->flags & ML_CLIP_MIDTEX) ||
				(curline->sidedef->Flags & WALLF_CLIP_MIDTEX))
			{
				ClipMidtex(x1, x2);
			}
		}

		mfloorclip = walllower;
		mceilingclip = wallupper;

		// draw the columns one at a time
		if (drawmode == DoDraw0)
		{
			for (dc_x = x1; dc_x <= x2; ++dc_x)
			{
				BlastMaskedColumn (R_DrawMaskedColumn, tex);
			}
		}
		else
		{
			// [RH] Draw up to four columns at once
			int stop = (x2+1) & ~3;

			if (x1 > x2)
				goto clearfog;

			dc_x = x1;

			while ((dc_x < stop) && (dc_x & 3))
			{
				BlastMaskedColumn (R_DrawMaskedColumn, tex);
				dc_x++;
			}

			while (dc_x < stop)
			{
				rt_initcols();
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, tex); dc_x++;
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, tex); dc_x++;
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, tex); dc_x++;
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, tex);
				rt_draw4cols (dc_x - 3);
				dc_x++;
			}

			while (dc_x <= x2)
			{
				BlastMaskedColumn (R_DrawMaskedColumn, tex);
				dc_x++;
			}
		}
	}
	else
	{ // Texture does wrap vertically.
		WallC.sz1 = ds->sz1;
		WallC.sz2 = ds->sz2;
		WallC.sx1 = ds->sx1;
		WallC.sx2 = ds->sx2;

		if (CurrentSkybox)
		{ // Midtex clipping doesn't work properly with skyboxes, since you're normally below the floor
		  // or above the ceiling, so the appropriate end won't be clipped automatically when adding
		  // this drawseg.
			if ((curline->linedef->flags & ML_CLIP_MIDTEX) ||
				(curline->sidedef->Flags & WALLF_CLIP_MIDTEX))
			{
				ClipMidtex(x1, x2);
			}
		}

		if (fake3D & FAKE3D_CLIPTOP)
		{
			OWallMost(wallupper, sclipTop - viewz, &WallC);
			for (i = x1; i <= x2; i++)
			{
				if (wallupper[i] < mceilingclip[i])
					wallupper[i] = mceilingclip[i];
			}
			mceilingclip = wallupper;
		}			
		if (fake3D & FAKE3D_CLIPBOTTOM)
		{
			OWallMost(walllower, sclipBottom - viewz, &WallC);
			for (i = x1; i <= x2; i++)
			{
				if (walllower[i] > mfloorclip[i])
					walllower[i] = mfloorclip[i];
			}
			mfloorclip = walllower;
		}

		rw_offset = 0;
		rw_pic = tex;
		wallscan_np2_ds(ds, x1, x2, mceilingclip, mfloorclip, MaskedSWall, maskedtexturecol, ds->yrepeat);
	}

clearfog:
	R_FinishSetPatchStyle ();
	if (ds->bFakeBoundary & 3)
	{
		R_RenderFakeWallRange(ds, x1, x2);
	}
	if (!notrelevant)
	{
		if (fake3D & FAKE3D_REFRESHCLIP)
		{
			assert(ds->bkup >= 0);
			memcpy(openings + ds->sprtopclip, openings + ds->bkup, (ds->x2-ds->x1+1) * 2);
		}
		else
		{
			clearbufshort(openings + ds->sprtopclip - ds->x1 + x1, x2-x1+1, viewheight);
		}
	}
	return;
}

// kg3D - render one fake wall
void R_RenderFakeWall(drawseg_t *ds, int x1, int x2, F3DFloor *rover)
{
	int i;
	fixed_t xscale, yscale;

	fixed_t Alpha = Scale(rover->alpha, OPAQUE, 255);
	ESPSResult drawmode;
	drawmode = R_SetPatchStyle (LegacyRenderStyles[rover->flags & FF_ADDITIVETRANS ? STYLE_Add : STYLE_Translucent],
		Alpha,	0, 0);

	if(drawmode == DontDraw) {
		R_FinishSetPatchStyle();
		return;
	}

	rw_lightstep = ds->lightstep;
	rw_light = ds->light + (x1 - ds->x1) * rw_lightstep;

	mfloorclip = openings + ds->sprbottomclip - ds->x1;
	mceilingclip = openings + ds->sprtopclip - ds->x1;

	spryscale = ds->iscale + ds->iscalestep * (x1 - ds->x1);
	rw_scalestep = ds->iscalestep;
	MaskedSWall = (fixed_t *)(openings + ds->swall) - ds->x1;

	// find positioning
	xscale = FixedMul(rw_pic->xScale, sidedef->GetTextureXScale(side_t::mid));
	yscale = FixedMul(rw_pic->yScale, sidedef->GetTextureYScale(side_t::mid));
	// encapsulate the lifetime of rowoffset
	fixed_t rowoffset = curline->sidedef->GetTextureYOffset(side_t::mid) + rover->master->sidedef[0]->GetTextureYOffset(side_t::mid);
	dc_texturemid = rover->model->GetPlaneTexZ(sector_t::ceiling);
	rw_offset = curline->sidedef->GetTextureXOffset(side_t::mid) + rover->master->sidedef[0]->GetTextureXOffset(side_t::mid);
	if (rowoffset < 0)
	{
		rowoffset += rw_pic->GetHeight() << FRACBITS;
	}
	if (rw_pic->bWorldPanning)
	{
		// rowoffset is added before the MulScale3 so that the masked texture will
		// still be positioned in world units rather than texels.

		dc_texturemid = MulScale16(dc_texturemid - viewz + rowoffset, yscale);
		rw_offset = MulScale16 (rw_offset, xscale);
	}
	else
	{
		// rowoffset is added outside the multiply so that it positions the texture
		// by texels instead of world units.
		dc_texturemid = MulScale16(dc_texturemid - viewz, yscale) + rowoffset;
	}

	if (fixedlightlev >= 0)
		dc_colormap = basecolormap->Maps + fixedlightlev;
	else if (fixedcolormap != NULL)
		dc_colormap = fixedcolormap;

	WallC.sz1 = ds->sz1;
	WallC.sz2 = ds->sz2;
	WallC.sx1 = ds->sx1;
	WallC.sx2 = ds->sx2;
	WallC.tx1 = ds->cx;
	WallC.ty1 = ds->cy;
	WallC.tx2 = ds->cx + ds->cdx;
	WallC.ty2 = ds->cy + ds->cdy;
	WallT = ds->tmapvals;

	OWallMost(wallupper, sclipTop - viewz, &WallC);
	OWallMost(walllower, sclipBottom - viewz, &WallC);

	for (i = x1; i <= x2; i++)
	{
		if (wallupper[i] < mceilingclip[i])
			wallupper[i] = mceilingclip[i];
	}
	for (i = x1; i <= x2; i++)
	{
		if (walllower[i] > mfloorclip[i])
			walllower[i] = mfloorclip[i];
	}

	PrepLWall (lwall, curline->sidedef->TexelLength*xscale, ds->sx1, ds->sx2);
	wallscan_np2_ds(ds, x1, x2, wallupper, walllower, MaskedSWall, lwall, yscale);
	R_FinishSetPatchStyle();
}

// kg3D - walls of fake floors
void R_RenderFakeWallRange (drawseg_t *ds, int x1, int x2)
{
	FTexture *const DONT_DRAW = ((FTexture*)(intptr_t)-1);
	int i,j;
	F3DFloor *rover, *fover = NULL;
	int passed, last;
	fixed_t floorheight;
	fixed_t ceilingheight;

	sprflipvert = false;
	curline = ds->curline;

	frontsector = curline->frontsector;
	backsector = curline->backsector;

	if (backsector == NULL)
	{
		return;
	}
	if ((ds->bFakeBoundary & 3) == 2)
	{
		sector_t *sec = backsector;
		backsector = frontsector;
		frontsector = sec;
	}

	floorheight = backsector->CenterFloor();
	ceilingheight = backsector->CenterCeiling();

	// maybe fix clipheights
	if (!(fake3D & FAKE3D_CLIPBOTTOM)) sclipBottom = floorheight;
	if (!(fake3D & FAKE3D_CLIPTOP))    sclipTop = ceilingheight;

	// maybe not visible
	if (sclipBottom >= frontsector->CenterCeiling()) return;
	if (sclipTop <= frontsector->CenterFloor()) return;

	if (fake3D & FAKE3D_DOWN2UP)
	{ // bottom to viewz
		last = 0;
		for (i = backsector->e->XFloor.ffloors.Size() - 1; i >= 0; i--) 
		{
			rover = backsector->e->XFloor.ffloors[i];
			if (!(rover->flags & FF_EXISTS)) continue;

			// visible?
			passed = 0;
			if (!(rover->flags & FF_RENDERSIDES) ||
				rover->top.plane->a || rover->top.plane->b ||
				rover->bottom.plane->a || rover->bottom.plane->b ||
				rover->top.plane->Zat0() <= sclipBottom ||
				rover->bottom.plane->Zat0() >= ceilingheight ||
				rover->top.plane->Zat0() <= floorheight)
			{
				if (!i)
				{
					passed = 1;
				}
				else
				{
					continue;
				}
			}

			rw_pic = NULL;
			if (rover->bottom.plane->Zat0() >= sclipTop || passed) 
			{
				if (last)
				{
					break;
				}
				// maybe wall from inside rendering?
				fover = NULL;
				for (j = frontsector->e->XFloor.ffloors.Size() - 1; j >= 0; j--)
				{
					fover = frontsector->e->XFloor.ffloors[j];
					if (fover->model == rover->model)
					{ // never
						fover = NULL;
						break;
					}
					if (!(fover->flags & FF_EXISTS)) continue;
					if (!(fover->flags & FF_RENDERSIDES)) continue;
					// no sloped walls, it's bugged
					if (fover->top.plane->a || fover->top.plane->b || fover->bottom.plane->a || fover->bottom.plane->b) continue;

					// visible?
					if (fover->top.plane->Zat0() <= sclipBottom) continue; // no
					if (fover->bottom.plane->Zat0() >= sclipTop)
					{ // no, last possible
 						fover = NULL;
						break;
					}
					// it is, render inside?
					if (!(fover->flags & (FF_BOTHPLANES|FF_INVERTPLANES)))
					{ // no
						fover = NULL;
					}
					break;
				}
				// nothing
				if (!fover || j == -1)
				{
					break;
				}
				// correct texture
				if (fover->flags & rover->flags & FF_SWIMMABLE)
				{	// don't ever draw (but treat as something has been found)
					rw_pic = DONT_DRAW;
				}
				else if(fover->flags & FF_UPPERTEXTURE)
				{
					rw_pic = TexMan(curline->sidedef->GetTexture(side_t::top), true);
				}
				else if(fover->flags & FF_LOWERTEXTURE)
				{
					rw_pic = TexMan(curline->sidedef->GetTexture(side_t::bottom), true);
				}
				else
				{
					rw_pic = TexMan(fover->master->sidedef[0]->GetTexture(side_t::mid), true);
				}
			} 
			else if (frontsector->e->XFloor.ffloors.Size()) 
			{
				// maybe not visible?
				fover = NULL;
				for (j = frontsector->e->XFloor.ffloors.Size() - 1; j >= 0; j--)
				{
					fover = frontsector->e->XFloor.ffloors[j];
					if (fover->model == rover->model) // never
					{
						break;
					}
					if (!(fover->flags & FF_EXISTS)) continue;
					if (!(fover->flags & FF_RENDERSIDES)) continue;
					// no sloped walls, it's bugged
					if (fover->top.plane->a || fover->top.plane->b || fover->bottom.plane->a || fover->bottom.plane->b) continue;

					// visible?
					if (fover->top.plane->Zat0() <= sclipBottom) continue; // no
					if (fover->bottom.plane->Zat0() >= sclipTop)
					{ // visible, last possible
 						fover = NULL;
						break;
					}
					if ((fover->flags & FF_SOLID) == (rover->flags & FF_SOLID) &&
						!(!(fover->flags & FF_SOLID) && (fover->alpha == 255 || rover->alpha == 255))
					)
					{
						break;
					}
					if (fover->flags & rover->flags & FF_SWIMMABLE)
					{ // don't ever draw (but treat as something has been found)
						rw_pic = DONT_DRAW;
					}
					fover = NULL; // visible
					break;
				}
				if (fover && j != -1)
				{
					fover = NULL;
					last = 1;
					continue; // not visible
				}
			}
			if (!rw_pic) 
			{
				fover = NULL;
				if (rover->flags & FF_UPPERTEXTURE)
				{
					rw_pic = TexMan(curline->sidedef->GetTexture(side_t::top), true);
				}
				else if(rover->flags & FF_LOWERTEXTURE)
				{
					rw_pic = TexMan(curline->sidedef->GetTexture(side_t::bottom), true);
				}
				else
				{
					rw_pic = TexMan(rover->master->sidedef[0]->GetTexture(side_t::mid), true);
				}
			}
			// correct colors now
			basecolormap = frontsector->ColorMap;
			wallshade = ds->shade;
			if (fixedlightlev < 0)
			{
				if ((ds->bFakeBoundary & 3) == 2)
				{
					for (j = backsector->e->XFloor.lightlist.Size() - 1; j >= 0; j--)
					{
						if (sclipTop <= backsector->e->XFloor.lightlist[j].plane.Zat0())
						{
							lightlist_t *lit = &backsector->e->XFloor.lightlist[j];
							basecolormap = lit->extra_colormap;
							wallshade = LIGHT2SHADE(curline->sidedef->GetLightLevel(foggy, *lit->p_lightlevel, lit->lightsource == NULL) + r_actualextralight);
							break;
						}
					}
				}
				else
				{
					for (j = frontsector->e->XFloor.lightlist.Size() - 1; j >= 0; j--)
					{
						if (sclipTop <= frontsector->e->XFloor.lightlist[j].plane.Zat0())
						{
							lightlist_t *lit = &frontsector->e->XFloor.lightlist[j];
							basecolormap = lit->extra_colormap;
							wallshade = LIGHT2SHADE(curline->sidedef->GetLightLevel(foggy, *lit->p_lightlevel, lit->lightsource == NULL) + r_actualextralight);
							break;
						}
					}
				}
			}
			if (rw_pic != DONT_DRAW)
			{
				R_RenderFakeWall(ds, x1, x2, fover ? fover : rover);
			}
			else rw_pic = NULL;
			break;
		}
	}
	else
	{ // top to viewz
		for (i = 0; i < (int)backsector->e->XFloor.ffloors.Size(); i++)
		{
			rover = backsector->e->XFloor.ffloors[i];
			if (!(rover->flags & FF_EXISTS)) continue;

			// visible?
			passed = 0;
			if (!(rover->flags & FF_RENDERSIDES) ||
				rover->top.plane->a || rover->top.plane->b ||
				rover->bottom.plane->a || rover->bottom.plane->b ||
				rover->bottom.plane->Zat0() >= sclipTop ||
				rover->top.plane->Zat0() <= floorheight ||
				rover->bottom.plane->Zat0() >= ceilingheight)
			{
				if ((unsigned)i == backsector->e->XFloor.ffloors.Size() - 1)
				{
					passed = 1;
				}
				else
				{
					continue;
				}
			}
			rw_pic = NULL;
			if (rover->top.plane->Zat0() <= sclipBottom || passed)
			{ // maybe wall from inside rendering?
				fover = NULL;
				for (j = 0; j < (int)frontsector->e->XFloor.ffloors.Size(); j++)
				{
					fover = frontsector->e->XFloor.ffloors[j];
					if (fover->model == rover->model)
					{ // never
						fover = NULL;
						break;
					}
					if (!(fover->flags & FF_EXISTS)) continue;
					if (!(fover->flags & FF_RENDERSIDES)) continue;
					// no sloped walls, it's bugged
					if (fover->top.plane->a || fover->top.plane->b || fover->bottom.plane->a || fover->bottom.plane->b) continue;

					// visible?
					if (fover->bottom.plane->Zat0() >= sclipTop) continue; // no
					if (fover->top.plane->Zat0() <= sclipBottom)
					{ // no, last possible
 						fover = NULL;
						break;
					}
					// it is, render inside?
					if (!(fover->flags & (FF_BOTHPLANES|FF_INVERTPLANES)))
					{ // no
						fover = NULL;
					}
					break;
				}
				// nothing
				if (!fover || (unsigned)j == frontsector->e->XFloor.ffloors.Size())
				{
					break;
				}
				// correct texture
				if (fover->flags & rover->flags & FF_SWIMMABLE)
				{
					rw_pic = DONT_DRAW;	// don't ever draw (but treat as something has been found)
				}
				else if (fover->flags & FF_UPPERTEXTURE)
				{
					rw_pic = TexMan(curline->sidedef->GetTexture(side_t::top), true);
				}
				else if (fover->flags & FF_LOWERTEXTURE)
				{
					rw_pic = TexMan(curline->sidedef->GetTexture(side_t::bottom), true);
				}
				else
				{
					rw_pic = TexMan(fover->master->sidedef[0]->GetTexture(side_t::mid), true);
				}
			}
			else if (frontsector->e->XFloor.ffloors.Size())
			{ // maybe not visible?
				fover = NULL;
				for (j = 0; j < (int)frontsector->e->XFloor.ffloors.Size(); j++)
				{
					fover = frontsector->e->XFloor.ffloors[j];
					if (fover->model == rover->model)
					{ // never
						break;
					}
					if (!(fover->flags & FF_EXISTS)) continue;
					if (!(fover->flags & FF_RENDERSIDES)) continue;
					// no sloped walls, its bugged
					if(fover->top.plane->a || fover->top.plane->b || fover->bottom.plane->a || fover->bottom.plane->b) continue;

					// visible?
					if (fover->bottom.plane->Zat0() >= sclipTop) continue; // no
					if (fover->top.plane->Zat0() <= sclipBottom)
					{ // visible, last possible
 						fover = NULL;
						break;
					}
					if ((fover->flags & FF_SOLID) == (rover->flags & FF_SOLID) &&
						!(!(rover->flags & FF_SOLID) && (fover->alpha == 255 || rover->alpha == 255))
					)
					{
						break;
					}
					if (fover->flags & rover->flags & FF_SWIMMABLE)
					{ // don't ever draw (but treat as something has been found)
						rw_pic = DONT_DRAW;
					}
					fover = NULL; // visible
					break;
				}
				if (fover && (unsigned)j != frontsector->e->XFloor.ffloors.Size())
				{ // not visible
					break;
				}
			}
			if (rw_pic == NULL)
			{
				fover = NULL;
				if (rover->flags & FF_UPPERTEXTURE)
				{
					rw_pic = TexMan(curline->sidedef->GetTexture(side_t::top), true);
				}
				else if (rover->flags & FF_LOWERTEXTURE)
				{
					rw_pic = TexMan(curline->sidedef->GetTexture(side_t::bottom), true);
				}
				else
				{
					rw_pic = TexMan(rover->master->sidedef[0]->GetTexture(side_t::mid), true);
				}
			}
			// correct colors now
			basecolormap = frontsector->ColorMap;
			wallshade = ds->shade;
			if (fixedlightlev < 0)
			{
				if ((ds->bFakeBoundary & 3) == 2)
				{
					for (j = backsector->e->XFloor.lightlist.Size() - 1; j >= 0; j--)
					{
						if (sclipTop <= backsector->e->XFloor.lightlist[j].plane.Zat0())
						{
							lightlist_t *lit = &backsector->e->XFloor.lightlist[j];
							basecolormap = lit->extra_colormap;
							wallshade = LIGHT2SHADE(curline->sidedef->GetLightLevel(foggy, *lit->p_lightlevel, lit->lightsource != NULL) + r_actualextralight);
							break;
						}
					}
				}
				else
				{
					for (j = frontsector->e->XFloor.lightlist.Size() - 1; j >= 0; j--)
					{
						if(sclipTop <= frontsector->e->XFloor.lightlist[j].plane.Zat0())
						{
							lightlist_t *lit = &frontsector->e->XFloor.lightlist[j];
							basecolormap = lit->extra_colormap;
							wallshade = LIGHT2SHADE(curline->sidedef->GetLightLevel(foggy, *lit->p_lightlevel, lit->lightsource != NULL) + r_actualextralight);
							break;
						}
					}
				}
			}

			if (rw_pic != DONT_DRAW)
			{
				R_RenderFakeWall(ds, x1, x2, fover ? fover : rover);
			}
			else
			{
				rw_pic = NULL;
			}
			break;
		}
	}
	return;
}

// prevlineasm1 is like vlineasm1 but skips the loop if only drawing one pixel
inline fixed_t prevline1 (fixed_t vince, BYTE *colormap, int count, fixed_t vplce, const BYTE *bufplce, BYTE *dest)
{
	dc_iscale = vince;
	dc_colormap = colormap;
	dc_count = count;
	dc_texturefrac = vplce;
	dc_source = bufplce;
	dc_dest = dest;
	return doprevline1 ();
}

void wallscan (int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal,
			   fixed_t yrepeat, const BYTE *(*getcol)(FTexture *tex, int x))
{
}

void wallscan_striped (int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat)
{
}

static void call_wallscan(int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat, bool mask)
{
}

//=============================================================================
//
// wallscan_np2
//
// This is a wrapper around wallscan that helps it tile textures whose heights
// are not powers of 2. It divides the wall into texture-sized strips and calls
// wallscan for each of those. Since only one repetition of the texture fits
// in each strip, wallscan will not tile.
//
//=============================================================================

void wallscan_np2(int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat, fixed_t top, fixed_t bot, bool mask)
{
}

static void wallscan_np2_ds(drawseg_t *ds, int x1, int x2, short *uwal, short *dwal, fixed_t *swal, fixed_t *lwal, fixed_t yrepeat)
{
}

inline fixed_t mvline1 (fixed_t vince, BYTE *colormap, int count, fixed_t vplce, const BYTE *bufplce, BYTE *dest)
{
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

//
// R_RenderSegLoop
// Draws zero, one, or two textures for walls.
// Can draw or mark the starting pixel of floor and ceiling textures.
// CALLED: CORE LOOPING ROUTINE.
//
// [RH] Rewrote this to use Build's wallscan, so it's quite far
// removed from the original Doom routine.
//

void R_RenderSegLoop ()
{
}

void R_NewWall (bool needlights)
{
	fixed_t rowoffset, yrepeat;

	rw_markmirror = false;

	sidedef = curline->sidedef;
	linedef = curline->linedef;

	// mark the segment as visible for auto map
	if (!r_dontmaplines) linedef->flags |= ML_MAPPED;

	midtexture = toptexture = bottomtexture = 0;

	if (backsector == NULL)
	{
		// single sided line
		// a single sided line is terminal, so it must mark ends
		markfloor = markceiling = true;
		// [RH] Render mirrors later, but mark them now.
		if (linedef->special != Line_Mirror || !r_drawmirrors)
		{
			// [RH] Horizon lines do not need to be textured
			if (linedef->special != Line_Horizon)
			{
				midtexture = TexMan(sidedef->GetTexture(side_t::mid), true);
				rw_offset_mid = sidedef->GetTextureXOffset(side_t::mid);
				rowoffset = sidedef->GetTextureYOffset(side_t::mid);
				rw_midtexturescalex = sidedef->GetTextureXScale(side_t::mid);
				rw_midtexturescaley = sidedef->GetTextureYScale(side_t::mid);
				yrepeat = FixedMul(midtexture->yScale, rw_midtexturescaley);
				if (yrepeat >= 0)
				{ // normal orientation
					if (linedef->flags & ML_DONTPEGBOTTOM)
					{ // bottom of texture at bottom
						rw_midtexturemid = MulScale16(frontsector->GetPlaneTexZ(sector_t::floor) - viewz, yrepeat) + (midtexture->GetHeight() << FRACBITS);
					}
					else
					{ // top of texture at top
						rw_midtexturemid = MulScale16(frontsector->GetPlaneTexZ(sector_t::ceiling) - viewz, yrepeat);
						if (rowoffset < 0 && midtexture != NULL)
						{
							rowoffset += midtexture->GetHeight() << FRACBITS;
						}
					}
				}
				else
				{ // upside down
					rowoffset = -rowoffset;
					if (linedef->flags & ML_DONTPEGBOTTOM)
					{ // top of texture at bottom
						rw_midtexturemid = MulScale16(frontsector->GetPlaneTexZ(sector_t::floor) - viewz, yrepeat);
					}
					else
					{ // bottom of texture at top
						rw_midtexturemid = MulScale16(frontsector->GetPlaneTexZ(sector_t::ceiling) - viewz, yrepeat) + (midtexture->GetHeight() << FRACBITS);
					}
				}
				if (midtexture->bWorldPanning)
				{
					rw_midtexturemid += MulScale16(rowoffset, yrepeat);
				}
				else
				{
					// rowoffset is added outside the multiply so that it positions the texture
					// by texels instead of world units.
					rw_midtexturemid += rowoffset;
				}
			}
		}
		else
		{
			rw_markmirror = true;
		}
	}
	else
	{ // two-sided line
		// hack to allow height changes in outdoor areas

		rw_frontlowertop = frontsector->GetPlaneTexZ(sector_t::ceiling);

		if (frontsector->GetTexture(sector_t::ceiling) == skyflatnum &&
			backsector->GetTexture(sector_t::ceiling) == skyflatnum)
		{
			if (rw_havehigh)
			{ // front ceiling is above back ceiling
				memcpy (&walltop[WallC.sx1], &wallupper[WallC.sx1], (WallC.sx2 - WallC.sx1)*sizeof(walltop[0]));
				rw_havehigh = false;
			}
			else if (rw_havelow && frontsector->ceilingplane != backsector->ceilingplane)
			{ // back ceiling is above front ceiling
				// The check for rw_havelow is not Doom-compliant, but it avoids HoM that
				// would otherwise occur because there is space made available for this
				// wall but nothing to draw for it.
				// Recalculate walltop so that the wall is clipped by the back sector's
				// ceiling instead of the front sector's ceiling.
				WallMost (walltop, backsector->ceilingplane, &WallC);
			}
			// Putting sky ceilings on the front and back of a line alters the way unpegged
			// positioning works.
			rw_frontlowertop = backsector->GetPlaneTexZ(sector_t::ceiling);
		}

		if ((rw_backcz1 <= rw_frontfz1 && rw_backcz2 <= rw_frontfz2) ||
			(rw_backfz1 >= rw_frontcz1 && rw_backfz2 >= rw_frontcz2))
		{
			// closed door
			markceiling = markfloor = true;
		}
		else
		{
			markfloor = rw_mustmarkfloor
				|| backsector->floorplane != frontsector->floorplane
				|| backsector->lightlevel != frontsector->lightlevel
				|| backsector->GetTexture(sector_t::floor) != frontsector->GetTexture(sector_t::floor)

				// killough 3/7/98: Add checks for (x,y) offsets
				|| backsector->GetXOffset(sector_t::floor) != frontsector->GetXOffset(sector_t::floor)
				|| backsector->GetYOffset(sector_t::floor) != frontsector->GetYOffset(sector_t::floor)
				|| backsector->GetAlpha(sector_t::floor) != frontsector->GetAlpha(sector_t::floor)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through deep water
				|| frontsector->heightsec

				|| backsector->GetPlaneLight(sector_t::floor) != frontsector->GetPlaneLight(sector_t::floor)
				|| backsector->GetFlags(sector_t::floor) != frontsector->GetFlags(sector_t::floor)

				// [RH] Add checks for colormaps
				|| backsector->ColorMap != frontsector->ColorMap

				|| backsector->GetXScale(sector_t::floor) != frontsector->GetXScale(sector_t::floor)
				|| backsector->GetYScale(sector_t::floor) != frontsector->GetYScale(sector_t::floor)

				|| backsector->GetAngle(sector_t::floor) != frontsector->GetAngle(sector_t::floor)

				// kg3D - add fake lights
				|| (frontsector->e && frontsector->e->XFloor.lightlist.Size())
				|| (backsector->e && backsector->e->XFloor.lightlist.Size())

				|| (sidedef->GetTexture(side_t::mid).isValid() &&
					((linedef->flags & (ML_CLIP_MIDTEX|ML_WRAP_MIDTEX)) ||
					 (sidedef->Flags & (WALLF_CLIP_MIDTEX|WALLF_WRAP_MIDTEX))))
				;

			markceiling = (frontsector->GetTexture(sector_t::ceiling) != skyflatnum ||
				backsector->GetTexture(sector_t::ceiling) != skyflatnum) &&
				(rw_mustmarkceiling
				|| backsector->ceilingplane != frontsector->ceilingplane
				|| backsector->lightlevel != frontsector->lightlevel
				|| backsector->GetTexture(sector_t::ceiling) != frontsector->GetTexture(sector_t::ceiling)

				// killough 3/7/98: Add checks for (x,y) offsets
				|| backsector->GetXOffset(sector_t::ceiling) != frontsector->GetXOffset(sector_t::ceiling)
				|| backsector->GetYOffset(sector_t::ceiling) != frontsector->GetYOffset(sector_t::ceiling)
				|| backsector->GetAlpha(sector_t::ceiling) != frontsector->GetAlpha(sector_t::ceiling)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through fake ceilings
				|| (frontsector->heightsec && frontsector->GetTexture(sector_t::ceiling) != skyflatnum)

				|| backsector->GetPlaneLight(sector_t::ceiling) != frontsector->GetPlaneLight(sector_t::ceiling)
				|| backsector->GetFlags(sector_t::ceiling) != frontsector->GetFlags(sector_t::ceiling)

				// [RH] Add check for colormaps
				|| backsector->ColorMap != frontsector->ColorMap

				|| backsector->GetXScale(sector_t::ceiling) != frontsector->GetXScale(sector_t::ceiling)
				|| backsector->GetYScale(sector_t::ceiling) != frontsector->GetYScale(sector_t::ceiling)

				|| backsector->GetAngle(sector_t::ceiling) != frontsector->GetAngle(sector_t::ceiling)

				// kg3D - add fake lights
				|| (frontsector->e && frontsector->e->XFloor.lightlist.Size())
				|| (backsector->e && backsector->e->XFloor.lightlist.Size())

				|| (sidedef->GetTexture(side_t::mid).isValid() &&
					((linedef->flags & (ML_CLIP_MIDTEX|ML_WRAP_MIDTEX)) ||
					(sidedef->Flags & (WALLF_CLIP_MIDTEX|WALLF_WRAP_MIDTEX))))
				);
		}

		if (rw_havehigh)
		{ // top texture
			toptexture = TexMan(sidedef->GetTexture(side_t::top), true);

			rw_offset_top = sidedef->GetTextureXOffset(side_t::top);
			rowoffset = sidedef->GetTextureYOffset(side_t::top);
			rw_toptexturescalex = sidedef->GetTextureXScale(side_t::top);
			rw_toptexturescaley = sidedef->GetTextureYScale(side_t::top);
			yrepeat = FixedMul(toptexture->yScale, rw_toptexturescaley);
			if (yrepeat >= 0)
			{ // normal orientation
				if (linedef->flags & ML_DONTPEGTOP)
				{ // top of texture at top
					rw_toptexturemid = MulScale16(frontsector->GetPlaneTexZ(sector_t::ceiling) - viewz, yrepeat);
					if (rowoffset < 0 && toptexture != NULL)
					{
						rowoffset += toptexture->GetHeight() << FRACBITS;
					}
				}
				else
				{ // bottom of texture at bottom
					rw_toptexturemid = MulScale16(backsector->GetPlaneTexZ(sector_t::ceiling) - viewz, yrepeat) + (toptexture->GetHeight() << FRACBITS);
				}
			}
			else
			{ // upside down
				rowoffset = -rowoffset;
				if (linedef->flags & ML_DONTPEGTOP)
				{ // bottom of texture at top
					rw_toptexturemid = MulScale16(frontsector->GetPlaneTexZ(sector_t::ceiling) - viewz, yrepeat) + (toptexture->GetHeight() << FRACBITS);
				}
				else
				{ // top of texture at bottom
					rw_toptexturemid = MulScale16(backsector->GetPlaneTexZ(sector_t::ceiling) - viewz, yrepeat);
				}
			}
			if (toptexture->bWorldPanning)
			{
				rw_toptexturemid += MulScale16(rowoffset, yrepeat);
			}
			else
			{
				rw_toptexturemid += rowoffset;
			}
		}
		if (rw_havelow)
		{ // bottom texture
			bottomtexture = TexMan(sidedef->GetTexture(side_t::bottom), true);

			rw_offset_bottom = sidedef->GetTextureXOffset(side_t::bottom);
			rowoffset = sidedef->GetTextureYOffset(side_t::bottom);
			rw_bottomtexturescalex = sidedef->GetTextureXScale(side_t::bottom);
			rw_bottomtexturescaley = sidedef->GetTextureYScale(side_t::bottom);
			yrepeat = FixedMul(bottomtexture->yScale, rw_bottomtexturescaley);
			if (yrepeat >= 0)
			{ // normal orientation
				if (linedef->flags & ML_DONTPEGBOTTOM)
				{ // bottom of texture at bottom
					rw_bottomtexturemid = MulScale16(rw_frontlowertop - viewz, yrepeat);
				}
				else
				{ // top of texture at top
					rw_bottomtexturemid = MulScale16(backsector->GetPlaneTexZ(sector_t::floor) - viewz, yrepeat);
					if (rowoffset < 0 && bottomtexture != NULL)
					{
						rowoffset += bottomtexture->GetHeight() << FRACBITS;
					}
				}
			}
			else
			{ // upside down
				rowoffset = -rowoffset;
				if (linedef->flags & ML_DONTPEGBOTTOM)
				{ // top of texture at bottom
					rw_bottomtexturemid = MulScale16(rw_frontlowertop - viewz, yrepeat);
				}
				else
				{ // bottom of texture at top
					rw_bottomtexturemid = MulScale16(backsector->GetPlaneTexZ(sector_t::floor) - viewz, yrepeat) + (bottomtexture->GetHeight() << FRACBITS);
				}
			}
			if (bottomtexture->bWorldPanning)
			{
				rw_bottomtexturemid += MulScale16(rowoffset, yrepeat);
			}
			else
			{
				rw_bottomtexturemid += rowoffset;
			}
		}
	}

	// if a floor / ceiling plane is on the wrong side of the view plane,
	// it is definitely invisible and doesn't need to be marked.

	// killough 3/7/98: add deep water check
	if (frontsector->GetHeightSec() == NULL)
	{
		int planeside;

		planeside = frontsector->floorplane.PointOnSide(viewx, viewy, viewz);
		if (frontsector->floorplane.c < 0)	// 3D floors have the floor backwards
			planeside = -planeside;
		if (planeside <= 0)		// above view plane
			markfloor = false;

		if (frontsector->GetTexture(sector_t::ceiling) != skyflatnum)
		{
			planeside = frontsector->ceilingplane.PointOnSide(viewx, viewy, viewz);
			if (frontsector->ceilingplane.c > 0)	// 3D floors have the ceiling backwards
				planeside = -planeside;
			if (planeside <= 0)		// below view plane
				markceiling = false;
		}
	}

	FTexture *midtex = TexMan(sidedef->GetTexture(side_t::mid), true);

	segtextured = midtex != NULL || toptexture != NULL || bottomtexture != NULL;

	// calculate light table
	if (needlights && (segtextured || (backsector && IsFogBoundary(frontsector, backsector))))
	{
		lwallscale =
			midtex ? FixedMul(midtex->xScale, sidedef->GetTextureXScale(side_t::mid)) :
			toptexture ? FixedMul(toptexture->xScale, sidedef->GetTextureXScale(side_t::top)) :
			bottomtexture ? FixedMul(bottomtexture->xScale, sidedef->GetTextureXScale(side_t::bottom)) :
			FRACUNIT;

		PrepWall (swall, lwall, sidedef->TexelLength * lwallscale, WallC.sx1, WallC.sx2);

		if (fixedcolormap == NULL && fixedlightlev < 0)
		{
			wallshade = LIGHT2SHADE(curline->sidedef->GetLightLevel(foggy, frontsector->lightlevel)
				+ r_actualextralight);
			GlobVis = r_WallVisibility;
			rw_lightleft = SafeDivScale12 (GlobVis, WallC.sz1);
			rw_lightstep = (SafeDivScale12 (GlobVis, WallC.sz2) - rw_lightleft) / (WallC.sx2 - WallC.sx1);
		}
		else
		{
			rw_lightleft = FRACUNIT;
			rw_lightstep = 0;
		}
	}
}


//
// R_CheckDrawSegs
//

void R_CheckDrawSegs ()
{
	if (ds_p == &drawsegs[MaxDrawSegs])
	{ // [RH] Grab some more drawsegs
		size_t newdrawsegs = MaxDrawSegs ? MaxDrawSegs*2 : 32;
		ptrdiff_t firstofs = firstdrawseg - drawsegs;
		drawsegs = (drawseg_t *)M_Realloc (drawsegs, newdrawsegs * sizeof(drawseg_t));
		firstdrawseg = drawsegs + firstofs;
		ds_p = drawsegs + MaxDrawSegs;
		MaxDrawSegs = newdrawsegs;
		DPrintf ("MaxDrawSegs increased to %zu\n", MaxDrawSegs);
	}
}

//
// R_CheckOpenings
//

ptrdiff_t R_NewOpening (ptrdiff_t len)
{
	ptrdiff_t res = lastopening;
	lastopening += len;
	if ((size_t)lastopening > maxopenings)
	{
		do
			maxopenings = maxopenings ? maxopenings*2 : 16384;
		while ((size_t)lastopening > maxopenings);
		openings = (short *)M_Realloc (openings, maxopenings * sizeof(*openings));
		DPrintf ("MaxOpenings increased to %zu\n", maxopenings);
	}
	return res;
}


//
// R_StoreWallRange
// A wall segment will be drawn between start and stop pixels (inclusive).
//

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

static void PrepWallRoundFix(fixed_t *lwall, fixed_t walxrepeat, int x1, int x2)
{
}

void PrepWall (fixed_t *swall, fixed_t *lwall, fixed_t walxrepeat, int x1, int x2)
{
}

void PrepLWall (fixed_t *lwall, fixed_t walxrepeat, int x1, int x2)
{
}

// pass = 0: when seg is first drawn
//		= 1: drawing masked textures (including sprites)
// Currently, only pass = 0 is done or used

static void R_RenderDecal (side_t *wall, DBaseDecal *decal, drawseg_t *clipper, int pass)
{
}
