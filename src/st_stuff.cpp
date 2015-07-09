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
//		Cheat code. See *_sbar.cpp for status bars.
//
//-----------------------------------------------------------------------------

#include "d_protocol.h"
#include "gstrings.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "d_event.h"
#include "gi.h"
#include "d_net.h"
#include "doomstat.h"
#include "g_level.h"
#include "st_stuff.h"
#include "d_player.h"

EXTERN_CVAR (Bool, ticker);
EXTERN_CVAR (Bool, noisedebug);
EXTERN_CVAR (Int, am_cheat);

struct FCheatDef
{
	FString mName, mCCMD;
	EGameType mGame;
	unsigned mPos;
};

static bool DoingSTCheat;
static unsigned SCheatDefsSize;
static unsigned SCheatArgsLen;
static TArray<FCheatDef> SCheatDefs;
static BYTE SCheatArgs[8];

static const char *CmdCheatCommands[] =
{
	"god",
	"give",
	"take",
	"noclip",
	"puke",
	"echo",
	"echolocal",
	"setplayerhealth",
	"toggle",
	"setcheat",
	"test",
	"eval",
	"idclev"
};

CCMD(addcheat)
{
	if(ParsingKeyConf)
	{
		if(argv.argc() != 3 && argv.argc() != 4)
		{
			Printf("Usage: addcheat <ccmd> <name> [<game>]\n");
			return;
		}
		
		char *ccmd = argv[1];
		char *name = argv[2];
		
		// Limit the cheat ccmd to 512 chars
		if(strlen(ccmd) > 512)
		{
			ccmd[512] = 0;
		}
		
		// Limit the cheat name to 32 characters
		if(strlen(name) > 32)
		{
			name[32] = 0;
		}
		
		for(unsigned i = 0; i < SCheatDefsSize; i++)
		{
			if(SCheatDefs[i].mName.CompareNoCase(name) == 0)
			{
				SCheatDefs[i].mCCMD = ccmd;
				return;
			}
		}
		
		unsigned SCurCheatDef = SCheatDefs.Reserve(1);
		SCheatDefsSize = SCheatDefs.Size();
		SCheatDefs[SCurCheatDef].mName = name;
		SCheatDefs[SCurCheatDef].mCCMD = ccmd;
		SCheatDefs[SCurCheatDef].mGame = GAME_Any;
		SCheatDefs[SCurCheatDef].mPos = 0;
		
		if(argv.argc() == 4)
		{
			const char *game = argv[3];
			if(strnicmp(game, "doom", 4) == 0)
			{
				SCheatDefs[SCurCheatDef].mGame = GAME_Doom;
			}
			if(strnicmp(game, "heretic", 7) == 0)
			{
				SCheatDefs[SCurCheatDef].mGame = GAME_Heretic;
			}
			if(strnicmp(game, "hexen", 5) == 0)
			{
				SCheatDefs[SCurCheatDef].mGame = GAME_Hexen;
			}
			if(strnicmp(game, "strife", 6) == 0)
			{
				SCheatDefs[SCurCheatDef].mGame = GAME_Strife;
			}
			if(strnicmp(game, "chex", 4) == 0)
			{
				SCheatDefs[SCurCheatDef].mGame = GAME_Chex;
			}
			if(strnicmp(game, "raven", 5) == 0)
			{
				SCheatDefs[SCurCheatDef].mGame = GAME_Raven;
			}
			if(strnicmp(game, "doomchex", 8) == 0)
			{
				SCheatDefs[SCurCheatDef].mGame = GAME_DoomChex;
			}
			if(strnicmp(game, "doomstrifechex", 14) == 0)
			{
				SCheatDefs[SCurCheatDef].mGame = GAME_DoomStrifeChex;
			}
		}
	}
}

CCMD(clearcheats)
{
	if(ParsingKeyConf)
	{
		SCheatDefs.Clear();
	}
}

CCMD(setplayerhealth)
{
	if(DoingSTCheat)
	{
		long health = deh.GodHealth;
		
		if(argv.argc() != 1)
		{
			health = strtol(argv[1], NULL, 0);
		}
		
		if(who)
		{
			who->health = health;
		}
	}
}

// [marrub] Yes, this is not safe. No, I don't really care.
CCMD(setcheat)
{
	if(DoingSTCheat)
	{
		if(argv.argc() == 3)
		{
			FBaseCVar *var = FindCVar(argv[1], NULL);
			if(var == NULL)
			{
				var = new FStringCVar(argv[1], NULL, CVAR_AUTO | CVAR_UNSETTABLE | cvar_defflags);
			}
			var->CmdSet(argv[2]);
		}
		else if(argv.argc() == 4)
		{
			FBaseCVar *var = FindCVar(argv[1], NULL);
			if(var == NULL)
			{
				var = new FStringCVar(argv[1], NULL, CVAR_AUTO | CVAR_UNSETTABLE | cvar_defflags);
			}
			int intvar = var->GetGenericRep(CVAR_Int).Int;
			if(intvar > strtol(argv[3], NULL, 0)-1)
			{
				var->CmdSet(argv[2]);
			}
			else
			{
				UCVarValue nv; nv.Int = intvar + 1;
				var->SetGenericRep(nv, CVAR_Int);
			}
		}
	}
}


CVAR(Bool, allcheats, false, CVAR_ARCHIVE)

static bool CheckCmdCheat(const char *cmd)
{
	// We have to parse it to make sure the cheat isn't invalud
	const char *end;
	const char *beg;

	// Skip any beginning whitespace
	while(*cmd && *cmd <= ' ')
		cmd++;

	// Find end of the command name
	if(*cmd == '\"')
	{
		for (end = beg = cmd+1; *end && *end != '\"'; ++end);
	}
	else
	{
		beg = cmd;
		for (end = cmd+1; *end > ' '; ++end);
	}

	const size_t len = end - beg;

	int i;
	for(i = countof(CmdCheatCommands)-1; i >= 0; --i)
	{
		if(strnicmp(beg, CmdCheatCommands[i], len) == 0 && CmdCheatCommands[i][len] == 0)
		{
			break;
		}
	}
	if(i < 0)
	{
		Printf("Invalid command for cheat: %s\n", beg);
		return false;
	}
	
	DoingSTCheat = true;
	C_DoCommand(cmd);
	DoingSTCheat = false;
	
	return true;
}

static bool RunCmdCheat(FString a_ccmd)
{
	unsigned beg = 0;
	for(unsigned i = 0; i < a_ccmd.Len() + 1; i++)
	{
		if(a_ccmd[i] == ';' || a_ccmd[i] == '\0')
		{
			if(CheckCmdCheat(a_ccmd.Mid(beg, i - beg)) == false)
			{
				return false;
			}
			
			beg = i + 1;
		}
	}
	
	return true;
}

static bool RunCmdCheatArgs(FString a_ccmd, BYTE *a_args, unsigned a_argLen)
{
	unsigned beg = 0;
	FString ccmd = a_ccmd;
	for(unsigned i = 0; i < a_argLen; i++)
	{
		FString sub, rep = a_args[i];
		sub.Format("&%u", i+1);
		ccmd.Substitute(sub, rep);
	}
	for(unsigned i = 0; i < ccmd.Len() + 1; i++)
	{
		if(ccmd[i] == ';' || ccmd[i] == '\0')
		{
			if(CheckCmdCheat(ccmd.Mid(beg, i - beg)) == false)
			{
				return false;
			}
			
			beg = i + 1;
		}
	}
	
	return true;
}

bool ST_DoCmdCheat(const char *cmdcheat, bool fromccmd)
{
	for(unsigned i = 0; i < SCheatDefsSize; i++)
	{
		if(SCheatDefs[i].mGame == GAME_Any || SCheatDefs[i].mGame & gameinfo.gametype)
		{
			if(SCheatDefs[i].mName.CompareNoCase(cmdcheat) == 0)
			{
				return RunCmdCheat(SCheatDefs[i].mCCMD);
			}
		}
	}
	
	if(fromccmd)
	{
		Printf("Couldn't find CmdCheat \"%s\"\n", cmdcheat);
	}
	
	return false;
}

// Respond to keyboard input events, intercept cheats.
// [RH] Cheats eat the last keypress used to trigger them
bool ST_Responder (event_t *ev)
{
	bool eat = false;
	bool clear;
	if(ev->type == EV_KeyDown)
	{
		BYTE key = (BYTE)ev->data2;
		
		for(unsigned i = 0; i < SCheatDefsSize; i++)
		{
			clear = false;
			if(allcheats || SCheatDefs[i].mGame == GAME_Any || SCheatDefs[i].mGame & gameinfo.gametype)
			{
				unsigned pos = SCheatDefs[i].mPos;
				if(key == SCheatDefs[i].mName[pos] || SCheatDefs[i].mName[pos] == '.')
				{
					eat = true;
					
					if(SCheatDefs[i].mName[pos] == '.' && SCheatArgsLen < 8)
					{
						SCheatArgs[SCheatArgsLen++] = key;
					}
					
					if(pos < SCheatDefs[i].mName.Len() - 1)
					{
						SCheatDefs[i].mPos++;
					}
					else
					{
						RunCmdCheatArgs(SCheatDefs[i].mCCMD, SCheatArgs, SCheatArgsLen);
						SCheatArgsLen = 0;
						for(int j = 0; j < countof(SCheatArgs); j++)
						{
							SCheatArgs[j] = 0;
						}
						clear = true;
					}
				}
				else
				{
					clear = true;
				}
			}
			
			if(clear)
			{
				SCheatDefs[i].mPos = 0;
			}
		}
	}
	return eat;
}
