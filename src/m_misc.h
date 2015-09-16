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
//
//	  
//-----------------------------------------------------------------------------


#ifndef __M_MISC__
#define __M_MISC__

#include "basictypes.h"
#include "zstring.h"

class FConfigFile;
class FGameConfigFile;
class FIWadManager;

extern FGameConfigFile *GameConfig;

bool M_WriteFile (char const *name, void *source, int length);
int M_ReadFile (char const *name, BYTE **buffer);
int M_ReadFileMalloc (char const *name, BYTE **buffer);
void M_FindResponseFile (void);

// [RH] M_ScreenShot now accepts a filename parameter.
//		Pass a NULL to get the original behavior.
void M_ScreenShot (const char *filename);

void M_LoadDefaults ();

bool M_SaveDefaults (const char *filename);
void M_SaveCustomKeys (FConfigFile *config, char *section, char *subsection, size_t sublen);



FString M_ZLibError(int zerrnum);

// Get special directory paths (defined in m_specialpaths.cpp)

#ifdef __unix__
FString GetUserFile (const char *path);		// Prepends ~/.zdoom to path
#endif
FString M_GetCachePath(bool create);
FString M_GetAutoexecPath();
FString M_GetCajunPath(const char *filename);
FString M_GetConfigPath(bool for_reading);
FString M_GetScreenshotsPath();
FString M_GetSavegamesPath();

#endif
