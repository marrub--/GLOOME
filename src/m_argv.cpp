/*
** m_argv.cpp
** Manages command line arguments
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <string.h>
#include "m_argv.h"
#include "cmdlib.h"
#include "i_system.h"

IMPLEMENT_CLASS (DArgs)

//===========================================================================
//
// DArgs Default Constructor
//
//===========================================================================

DArgs::DArgs()
{
}

//===========================================================================
//
// DArgs Copy Constructor
//
//===========================================================================

DArgs::DArgs(const DArgs &other)
: DObject(), Argv(other.Argv)
{
}

//===========================================================================
//
// DArgs Argv Constructor
//
//===========================================================================

DArgs::DArgs(int argc, char **argv)
{
	SetArgs(argc, argv);
}

//===========================================================================
//
// DArgs String Argv Constructor
//
//===========================================================================

DArgs::DArgs(int argc, FString *argv)
{
	AppendArgs(argc, argv);
}



//===========================================================================
//
// DArgs Copy Operator
//
//===========================================================================

DArgs &DArgs::operator=(const DArgs &other)
{
	Argv = other.Argv;
	return *this;
}

//===========================================================================
//
// DArgs :: SetArgs
//
//===========================================================================

void DArgs::SetArgs(int argc, char **argv)
{
	Argv.Resize(argc);
	for (int i = 0; i < argc; ++i)
	{
		Argv[i] = argv[i];
	}
}

//===========================================================================
//
// DArgs :: FlushArgs
//
//===========================================================================

void DArgs::FlushArgs()
{
	Argv.Clear();
}

//===========================================================================
//
// DArgs :: CheckParm
//
// Checks for the given parameter in the program's command line arguments.
// Returns the argument number (1 to argc-1) or 0 if not present
//
//===========================================================================

int DArgs::CheckParm(const char *check, int start) const
{
	for (unsigned i = start; i < Argv.Size(); ++i)
	{
		if (0 == stricmp(check, Argv[i]))
		{
			return i;
		}
	}
	return 0;
}

//===========================================================================
//
// DArgs :: CheckParmList
//
// Returns the number of arguments after the parameter (if found) and also
// returns a pointer to the first argument.
//
//===========================================================================

int DArgs::CheckParmList(const char *check, FString **strings, int start) const
{
	unsigned int i, parmat = CheckParm(check, start);

	if (parmat == 0)
	{
		if (strings != NULL)
		{
			*strings = NULL;
		}
		return 0;
	}
	for (i = ++parmat; i < Argv.Size(); ++i)
	{
		if (Argv[i][0] == '-' || Argv[i][1] == '+')
		{
			break;
		}
	}
	if (strings != NULL)
	{
		*strings = &Argv[parmat];
	}
	return i - parmat;
}

//===========================================================================
//
// DArgs :: CheckValue
//
// Like CheckParm, but it also checks that the parameter has a value after
// it and returns that or NULL if not present.
//
//===========================================================================

const char *DArgs::CheckValue(const char *check) const
{
	int i = CheckParm(check);

	if (i > 0 && i < (int)Argv.Size() - 1)
	{
		i++;
		return Argv[i][0] != '+' && Argv[i][0] != '-' ? Argv[i].GetChars() : NULL;
	}
	else
	{
		return NULL;
	}
}

//===========================================================================
//
// DArgs :: TakeValue
//
// Like CheckValue, except it also removes the parameter and its argument
// (if present) from argv.
//
//===========================================================================

FString DArgs::TakeValue(const char *check)
{
	int i = CheckParm(check);
	FString out;

	if (i > 0 && i < (int)Argv.Size())
	{
		if (i < (int)Argv.Size() - 1 && Argv[i+1][0] != '+' && Argv[i+1][0] != '-')
		{
			out = Argv[i+1];
			Argv.Delete(i, 2);	// Delete the parm and its value.
		}
		else
		{
			Argv.Delete(i);		// Just delete the parm, since it has no value.
		}
	}
	return out;
}

//===========================================================================
//
// DArgs :: RemoveArg
//
//===========================================================================

void DArgs::RemoveArgs(const char *check)
{
	int i = CheckParm(check);

	if (i > 0 && i < (int)Argv.Size() - 1)
	{
		do 
		{
			RemoveArg(i);
		}
		while (Argv[i][0] != '+' && Argv[i][0] != '-' && i < (int)Argv.Size() - 1);
	}
}

//===========================================================================
//
// DArgs :: GetArg
//
// Gets the argument at a particular position.
//
//===========================================================================

const char *DArgs::GetArg(int arg) const
{
	return ((unsigned)arg < Argv.Size()) ? Argv[arg].GetChars() : NULL;
}

//===========================================================================
//
// DArgs :: GetArgList
//
// Returns a pointer to the FString at a particular position.
//
//===========================================================================

FString *DArgs::GetArgList(int arg) const
{
	return ((unsigned)arg < Argv.Size()) ? &Argv[arg] : NULL;
}

//===========================================================================
//
// DArgs :: NumArgs
//
//===========================================================================

int DArgs::NumArgs() const
{
	return (int)Argv.Size();
}

//===========================================================================
//
// DArgs :: AppendArg
//
// Adds another argument to argv. Invalidates any previous results from
// GetArgList().
//
//===========================================================================

void DArgs::AppendArg(FString arg)
{
	Argv.Push(arg);
}

//===========================================================================
//
// DArgs :: AppendArgs
//
// Adds an array of FStrings to argv.
//
//===========================================================================

void DArgs::AppendArgs(int argc, const FString *argv)
{
	if (argv != NULL && argc > 0)
	{
		Argv.Grow(argc);
		for (int i = 0; i < argc; ++i)
		{
			Argv.Push(argv[i]);
		}
	}
}

//===========================================================================
//
// DArgs :: RemoveArg
//
// Removes a single argument from argv.
//
//===========================================================================

void DArgs::RemoveArg(int argindex)
{
	Argv.Delete(argindex);
}

//===========================================================================
//
// DArgs :: CollectFiles
//
// Takes all arguments after any instance of -param and any arguments before
// all switches that end in .extension and combines them into a single
// -switch block at the end of the arguments. If extension is NULL, then
// every parameter before the first switch is added after this -param.
//
//===========================================================================

void DArgs::CollectFiles(const char *param, const char *extension)
{
	TArray<FString> work;
	unsigned int i;
	size_t extlen = extension == NULL ? 0 : strlen(extension);

	// Step 1: Find suitable arguments before the first switch.
	i = 1;
	while (i < Argv.Size() && Argv[i][0] != '-' && Argv[i][0] != '+')
	{
		bool useit;

		if (extlen > 0)
		{ // Argument's extension must match.
			size_t len = Argv[i].Len();
			useit = (len >= extlen && stricmp(&Argv[i][len - extlen], extension) == 0);
		}
		else
		{ // Anything will do so long as it's before the first switch.
			useit = true;
		}
		if (useit)
		{
			work.Push(Argv[i]);
			Argv.Delete(i);
		}
		else
		{
			i++;
		}
	}

	// Step 2: Find each occurence of -param and add its arguments to work.
	while ((i = CheckParm(param, i)) > 0)
	{
		Argv.Delete(i);
		while (i < Argv.Size() && Argv[i][0] != '-' && Argv[i][0] != '+')
		{
			work.Push(Argv[i]);
			Argv.Delete(i);
		}
	}

	// Optional: Replace short path names with long path names
#ifdef _WIN32
	for (i = 0; i < work.Size(); ++i)
	{
		work[i] = I_GetLongPathName(work[i]);
	}
#endif

	// Step 3: Add work back to Argv, as long as it's non-empty.
	if (work.Size() > 0)
	{
		Argv.Push(param);
		AppendArgs(work.Size(), &work[0]);
	}
}

//===========================================================================
//
// DArgs :: GatherFiles
//
// Returns all the arguments after the first instance of -param. If you want
// to combine more than one or get switchless stuff included, you need to
// call CollectFiles first.
//
//===========================================================================

DArgs *DArgs::GatherFiles(const char *param) const
{
	FString *files;
	int filecount;

	filecount = CheckParmList(param, &files);
	return new DArgs(filecount, files);
}
