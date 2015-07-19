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
// $Log:$
//
// DESCRIPTION:
//		Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include "templates.h"
#include "doomdef.h"
#include "m_swap.h"
#include "i_system.h"
#include "w_wad.h"
#include "r_local.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "doomstat.h"
#include "v_video.h"
#include "sc_man.h"
#include "s_sound.h"
#include "sbar.h"
#include "gi.h"
#include "r_sky.h"
#include "cmdlib.h"
#include "g_level.h"
#include "d_net.h"
#include "colormatcher.h"
#include "d_netinf.h"
#include "p_effect.h"
#include "r_bsp.h"
#include "r_plane.h"
#include "r_segs.h"
#include "r_3dfloors.h"
#include "v_palette.h"
#include "r_data/r_translate.h"
#include "r_data/colormaps.h"
#include "r_data/voxels.h"
#include "p_local.h"

fixed_t 		pspritexscale;
fixed_t			pspriteyscale;
fixed_t 		pspritexiscale;
fixed_t			sky1scale;
fixed_t			sky2scale;

vissprite_t		*VisPSprites[NUMPSPRITES];
int				VisPSpritesX1[NUMPSPRITES];
FDynamicColormap *VisPSpritesBaseColormap[NUMPSPRITES];

FTexture		*WallSpriteTile;

short			zeroarray[MAXWIDTH];
short			screenheightarray[MAXWIDTH];

int OffscreenBufferWidth, OffscreenBufferHeight;
BYTE *OffscreenColorBuffer;

int				MaxVisSprites;
vissprite_t 	**vissprites;
vissprite_t		**firstvissprite;
vissprite_t		**vissprite_p;
vissprite_t		**lastvissprite;
int 			newvissprite;
bool			DrewAVoxel;


void R_DeinitSprites()
{
}

void R_ClearSprites (void)
{
}

vissprite_t *R_NewVisSprite (void)
{
	return NULL;
}

short*			mfloorclip;
short*			mceilingclip;

fixed_t 		spryscale;
fixed_t 		sprtopscreen;

bool			sprflipvert;

void R_DrawMaskedColumn (const BYTE *column, const FTexture::Span *span)
{
}

void R_DrawVisSprite (vissprite_t *vis)
{
}

void R_DrawWallSprite(vissprite_t *spr)
{
}

void R_WallSpriteColumn (void (*drawfunc)(const BYTE *column, const FTexture::Span *spans))
{
}

void R_DrawVisVoxel(vissprite_t *spr, int minslabz, int maxslabz, short *cliptop, short *clipbot)
{
}

void R_ProjectSprite (AActor *thing, int fakeside, F3DFloor *fakefloor, F3DFloor *fakeceiling)
{
}

void R_AddSprites (sector_t *sec, int lightlevel, int fakeside)
{
}

void R_DrawPSprite (pspdef_t* psp, int pspnum, AActor *owner, fixed_t sx, fixed_t sy)
{
}

void R_DrawPlayerSprites ()
{
}

void R_DrawRemainingPlayerSprites()
{
}

void R_SortVisSprites (bool (*compare)(vissprite_t *, vissprite_t *), size_t first)
{
}

void R_DrawSprite (vissprite_t *spr)
{
}

void R_DrawMaskedSingle (bool renew)
{
}

void R_DrawMasked (void)
{
}

void R_ProjectParticle (particle_t *particle, const sector_t *sector, int shade, int fakeside)
{
}

void R_DrawParticle (vissprite_t *vis)
{
}

void R_DrawVoxel(fixed_t globalposx, fixed_t globalposy, fixed_t globalposz, angle_t viewang,
	fixed_t dasprx, fixed_t daspry, fixed_t dasprz, angle_t dasprang,
	fixed_t daxscale, fixed_t dayscale, FVoxel *voxobj,
	lighttable_t *colormap, short *daumost, short *dadmost, int minslabz, int maxslabz, int flags)
{
}

void R_CheckOffscreenBuffer(int width, int height, bool spansonly)
{
}
