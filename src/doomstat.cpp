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
// $Log:$
//
// DESCRIPTION:
//      Put all global state variables here.
//
//-----------------------------------------------------------------------------



#include "stringtable.h"
#include "doomstat.h"
#include "c_cvars.h"
#include "i_system.h"
#include "g_level.h"
#include "p_local.h"

int SaveVersion;

// Localizable strings
FStringTable	GStrings;

// Game speed
EGameSpeed		GameSpeed = SPEED_Normal;

// Show developer messages if true.
CVAR (Bool, developer, false, 0)

// [RH] Feature control cvars
CVAR (Bool, var_friction, true, CVAR_SERVERINFO);
CVAR (Bool, var_pushers, true, CVAR_SERVERINFO);

CVAR (Bool, alwaysapplydmflags, false, CVAR_SERVERINFO);

CUSTOM_CVAR (Float, teamdamage, 0.f, CVAR_SERVERINFO)
{
	level.teamdamage = self;
}

CUSTOM_CVAR (String, language, "auto", CVAR_ARCHIVE)
{
	SetLanguageIDs ();
	GStrings.LoadStrings (false);
	if (level.info != NULL) level.LevelName = level.info->LookupLevelName();
}

// [RH] Network arbitrator
int Net_Arbitrator = 0;

int NextSkill = -1;

int SinglePlayerClass[MAXPLAYERS];

bool ToggleFullscreen;
int BorderTopRefresh;

FString LumpFilterIWAD;
