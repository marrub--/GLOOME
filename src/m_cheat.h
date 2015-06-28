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
//		Cheat code checking.
//
//-----------------------------------------------------------------------------


#ifndef __M_CHEAT_H__
#define __M_CHEAT_H__

//
// CHEAT SEQUENCE PACKAGE
//

// [RH] Functions that actually perform the cheating
class player_t;
struct PClass;
void cht_DoCheat (player_t *player, int cheat);
void cht_Give (player_t *player, const char *item, int amount=1);
void cht_Take (player_t *player, const char *item, int amount=1);
void cht_Suicide (player_t *player);
const char *cht_Morph (player_t *player, const PClass *morphclass, bool quickundo);

#endif
