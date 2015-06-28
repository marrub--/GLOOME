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
//		Sky rendering.
//
//-----------------------------------------------------------------------------

#ifndef __R_SKY_H__
#define __R_SKY_H__

#include "textures/textures.h"


extern FTextureID	skyflatnum;
extern fixed_t		sky1cyl,		sky2cyl;
extern FTextureID	sky1texture,	sky2texture;
extern double		sky1pos,		sky2pos;
extern fixed_t	skytexturemid;
extern fixed_t	skyiscale;
extern fixed_t	skyscale;
extern bool		skystretch;
extern fixed_t freelookviewheight;

#define SKYSTRETCH_HEIGHT 228

// Called whenever the sky changes.
void R_InitSkyMap		();
void R_UpdateSky (DWORD mstime);

#endif //__R_SKY_H__
