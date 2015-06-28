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
//		Lookup tables.
//		Do not try to look them up :-).
//		In the order of appearance: 
//
//		int finetangent[4096]	- Tangens LUT.
//		 Should work with BAM fairly well (12 of 16bit,
//		effectively, by shifting).
//
//		int finesine[10240] 			- Sine lookup.
//		 Guess what, serves as cosine, too.
//		 Remarkable thing is, how to use BAMs with this? 
//
//		int tantoangle[2049]	- ArcTan LUT,
//		  maps tan(angle) to angle fast. Gotta search.
//		
//	  
//-----------------------------------------------------------------------------

#include "tables.h"

fixed_t finetangent[4096];
fixed_t finesine[10240];
angle_t tantoangle[2049];

cosine_inline finecosine;	// in case this is actually needed
