// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_net.h,v 1.1.1.1 1997/12/28 12:59:02 pekangas Exp $
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
//		System specific network interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __I_NET_H__
#define __I_NET_H__

// Called by D_DoomMain.
bool I_InitNetwork (void);
void I_NetCmd (void);

#endif
