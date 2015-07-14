// Emacs style mode select	 -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (c) 2015 Graham Sanderson <marrub@greyserv.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Log:$
//
// DESCRIPTION:
//		Does event handling for ACS.
//
//-----------------------------------------------------------------------------
#include "templates.h"
#include "doomdef.h"
#include "p_local.h"
#include "p_spec.h"
#include "g_level.h"
#include "s_sound.h"
#include "g_acsresponder.h"
#include "p_acs.h"
#include "doomstat.h"
#include "d_player.h"
#include "c_bind.h"

bool ACS_Responder(event_t *ev)
{
	if(ev->type != EV_KeyDown)
	{
		return false;
	}

	if(ev->type == EV_KeyDown && FBehavior::ACSKeyLock)
	{
		if(!Bindings.GetBinding(ev->data1).CompareNoCase("toggleconsole"))
		{
			return false;
		}

		FBehavior::StaticStartTypedScripts(SCRIPT_Input, players[consoleplayer].mo, true, ev->data1, true);
		return true;
	}

	return FBehavior::ACSKeyLock;
}
