// Emacs style mode select   -*- C++ -*- 
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
//		System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __I_VIDEO_H__
#define __I_VIDEO_H__


class DFrameBuffer;


// [RH] Set the display mode
DFrameBuffer *I_SetMode (int &width, int &height, DFrameBuffer *old);

// Pause a bit.
// [RH] Despite the name, it apparently never waited for the VBL, even in
// the original DOS version (if the Heretic/Hexen source is any indicator).
void I_WaitVBL(int count);

bool I_CheckResolution (int width, int height, int bpp);
void I_ClosestResolution (int *width, int *height, int bits);

enum EDisplayType
{
	DISPLAY_WindowOnly,
	DISPLAY_FullscreenOnly,
	DISPLAY_Both
};

#endif // __I_VIDEO_H__
