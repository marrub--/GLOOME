/*
**	r_3dfloors.cpp
**	software 3D floors addon
**
**	by kgsws
*/

#include "templates.h"
#include "doomdef.h"
#include "p_local.h"
#include "c_dispatch.h"
#include "r_local.h"
#include "r_bsp.h"
#include "r_plane.h"
#include "c_cvars.h"
#include "r_3dfloors.h"

int fake3D;
F3DFloor *fakeFloor;
fixed_t fakeHeight;
fixed_t fakeAlpha;
int fakeActive = 0;
fixed_t sclipBottom;
fixed_t sclipTop;
HeightLevel *height_top = NULL;
HeightLevel *height_cur = NULL;
int CurrentMirror = 0;
int CurrentSkybox = 0;

CVAR(Int, r_3dfloors, true, 0);

int height_max = -1;
TArray<HeightStack> toplist;
ClipStack *clip_top = NULL;
ClipStack *clip_cur = NULL;

void R_3D_DeleteHeights()
{
}

void R_3D_AddHeight(secplane_t *add, sector_t *sec)
{
}

void R_3D_NewClip()
{
}

void R_3D_ResetClip()
{
}

void R_3D_EnterSkybox()
{
}

void R_3D_LeaveSkybox()
{
}

