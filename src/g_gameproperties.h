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
//		Networked game properties.
//
//-----------------------------------------------------------------------------

#ifndef GLOOME_G_GAMEPROPERTIES_H
#define GLOOME_G_GAMEPROPERTIES_H

#include "tarray.h"

class FGameProperties
{
	bool m_DeathRestarts;
	bool m_SavesEnabled;
public:
	enum EGameProperty
	{
		GPROP_DeathRestarts,
		GPROP_SavesEnabled,
	};

	FGameProperties()
	{
		m_DeathRestarts = true;
		m_SavesEnabled = true;
	}

	~FGameProperties()
	{
	}

	// returns: success
	bool SetGameProperty(int gp, int value)
	{
		// [marrub] TODO: until I start caring about networking, just set them
		switch(gp)
		{
			case FGameProperties::GPROP_DeathRestarts: m_DeathRestarts = !!value; return true;
			case FGameProperties::GPROP_SavesEnabled: m_SavesEnabled = !!value; return true;
		}

		return false;
	}

	// returns: property
	int GetGameProperty(int gp)
	{
		// [marrub] TODO: until I start caring about networking, just get them
		switch(gp)
		{
			case FGameProperties::GPROP_DeathRestarts: return m_DeathRestarts;
			case FGameProperties::GPROP_SavesEnabled: return m_SavesEnabled;
		}

		return -1;
	}
};

extern FGameProperties ngameproperties;

#endif

