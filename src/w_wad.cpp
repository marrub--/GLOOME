/*
** w_wad.cpp
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** Copyright 2005-2009 Christoph Oelckers
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
**
*/


// HEADER FILES ------------------------------------------------------------

#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "doomtype.h"
#include "m_argv.h"
#include "i_system.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "w_wad.h"
#include "w_zip.h"
#include "m_crc32.h"
#include "v_text.h"
#include "templates.h"
#include "gi.h"
#include "doomerrors.h"
#include "resourcefiles/resourcefile.h"
#include "md5.h"
#include "doomstat.h"

// MACROS ------------------------------------------------------------------

#define NULL_INDEX		(0xffffffff)

//
// WADFILE I/O related stuff.
//
struct FWadCollection::LumpRecord
{
	int			wadnum;
	FResourceLump *lump;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------
extern bool nospriterename;

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void PrintLastError ();

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FWadCollection Wads;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// uppercoppy
//
// [RH] Copy up to 8 chars, upper-casing them in the process
//==========================================================================

void uppercopy (char *to, const char *from)
{
	int i;

	for (i = 0; i < 8 && from[i]; i++)
		to[i] = toupper (from[i]);
	for (; i < 8; i++)
		to[i] = 0;
}

FWadCollection::FWadCollection ()
: FirstLumpIndex(NULL), NextLumpIndex(NULL),
  FirstLumpIndex_FullName(NULL), NextLumpIndex_FullName(NULL), 
  NumLumps(0)
{
}

FWadCollection::~FWadCollection ()
{
	DeleteAll();
}

void FWadCollection::DeleteAll ()
{
	if (FirstLumpIndex != NULL)
	{
		delete[] FirstLumpIndex;
		FirstLumpIndex = NULL;
	}
	if (NextLumpIndex != NULL)
	{
		delete[] NextLumpIndex;
		NextLumpIndex = NULL;
	}
	if (FirstLumpIndex_FullName != NULL)
	{
		delete[] FirstLumpIndex_FullName;
		FirstLumpIndex_FullName = NULL;
	}
	if (NextLumpIndex_FullName != NULL)
	{
		delete[] NextLumpIndex_FullName;
		NextLumpIndex_FullName = NULL;
	}

	LumpInfo.Clear();
	NumLumps = 0;

	// we must count backward to enssure that embedded WADs are deleted before
	// the ones that contain their data.
	for (int i = Files.Size() - 1; i >= 0; --i)
	{
		delete Files[i];
	}
	Files.Clear();
}

//==========================================================================
//
// W_InitMultipleFiles
//
// Pass a null terminated list of files to use. All files are optional,
// but at least one file must be found. Lump names can appear multiple
// times. The name searcher looks backwards, so a later file can
// override an earlier one.
//
//==========================================================================

void FWadCollection::InitMultipleFiles (TArray<FString> &filenames)
{
	int numfiles;

	// open all the files, load headers, and count lumps
	DeleteAll();
	numfiles = 0;

	for(unsigned i=0;i<filenames.Size(); i++)
	{
		int baselump = NumLumps;
		AddFile (filenames[i]);
	}

	NumLumps = LumpInfo.Size();
	if (NumLumps == 0)
	{
		I_FatalError ("W_InitMultipleFiles: no files found");
	}
	RenameNerve();
	RenameSprites();

	// [RH] Set up hash table
	FirstLumpIndex = new DWORD[NumLumps];
	NextLumpIndex = new DWORD[NumLumps];
	FirstLumpIndex_FullName = new DWORD[NumLumps];
	NextLumpIndex_FullName = new DWORD[NumLumps];
	InitHashChains ();
	LumpInfo.ShrinkToFit();
	Files.ShrinkToFit();
}

//-----------------------------------------------------------------------
//
// Adds an external file to the lump list but not to the hash chains
// It's just a simple means to assign a lump number to some file so that
// the texture manager can read from it.
//
//-----------------------------------------------------------------------

int FWadCollection::AddExternalFile(const char *filename)
{
	FResourceLump *lump = new FExternalLump(filename);

	FWadCollection::LumpRecord *lumprec = &LumpInfo[LumpInfo.Reserve(1)];
	lumprec->lump = lump;
	lumprec->wadnum = -1;
	return LumpInfo.Size()-1;	// later
}

//==========================================================================
//
// W_AddFile
//
// Files with a .wad extension are wadlink files with multiple lumps,
// other files are single lumps with the base filename for the lump name.
//
// [RH] Removed reload hack
//==========================================================================

void FWadCollection::AddFile (const char *filename, FileReader *wadinfo)
{
	int startlump;
	bool isdir = false;

	if (wadinfo == NULL)
	{
		// Does this exist? If so, is it a directory?
		struct stat info;
		if (stat(filename, &info) != 0)
		{
			Printf(TEXTCOLOR_RED "Could not stat %s\n", filename);
			PrintLastError();
			return;
		}
		isdir = (info.st_mode & S_IFDIR) != 0;

		if (!isdir)
		{
			try
			{
				wadinfo = new FileReader(filename);
			}
			catch (CRecoverableError &err)
			{ // Didn't find file
				Printf (TEXTCOLOR_RED "%s\n", err.GetMessage());
				PrintLastError ();
				return;
			}
		}
	}

	Printf (" adding %s", filename);
	startlump = NumLumps;

	FResourceFile *resfile;
	
	if (!isdir)
		resfile = FResourceFile::OpenResourceFile(filename, wadinfo);
	else
		resfile = FResourceFile::OpenDirectory(filename);

	if (resfile != NULL)
	{
		DWORD lumpstart = LumpInfo.Size();

		resfile->SetFirstLump(lumpstart);
		for (DWORD i=0; i < resfile->LumpCount(); i++)
		{
			FResourceLump *lump = resfile->GetLump(i);
			FWadCollection::LumpRecord *lump_p = &LumpInfo[LumpInfo.Reserve(1)];

			lump_p->lump = lump;
			lump_p->wadnum = Files.Size();
		}

		if (Files.Size() == IWAD_FILENUM && gameinfo.gametype == GAME_Strife && gameinfo.flags & GI_SHAREWARE)
		{
			resfile->FindStrifeTeaserVoices();
		}
		Files.Push(resfile);

		for (DWORD i=0; i < resfile->LumpCount(); i++)
		{
			FResourceLump *lump = resfile->GetLump(i);
			if (lump->Flags & LUMPF_EMBEDDED)
			{
				FString path;
				path.Format("%s:%s", filename, lump->FullName.GetChars());
				FileReader *embedded = lump->NewReader();
				AddFile(path, embedded);
			}
		}

		if (hashfile)
		{
			BYTE cksum[16];
			char cksumout[33];
			memset(cksumout, 0, sizeof(cksumout));

			FileReader *reader = wadinfo;

			if (reader != NULL)
			{
				MD5Context md5;
				reader->Seek(0, SEEK_SET);
				md5.Update(reader, reader->GetLength());
				md5.Final(cksum);

				for (size_t j = 0; j < sizeof(cksum); ++j)
				{
					sprintf(cksumout + (j * 2), "%02X", cksum[j]);
				}

				fprintf(hashfile, "file: %s, hash: %s, size: %ld\n", filename, cksumout, reader->GetLength());
			}

			else
				fprintf(hashfile, "file: %s, Directory structure\n", filename);

			for (DWORD i = 0; i < resfile->LumpCount(); i++)
			{
				FResourceLump *lump = resfile->GetLump(i);

				if (!(lump->Flags & LUMPF_EMBEDDED))
				{
					reader = lump->NewReader();

					MD5Context md5;
					md5.Update(reader, lump->LumpSize);
					md5.Final(cksum);

					for (size_t j = 0; j < sizeof(cksum); ++j)
					{
						sprintf(cksumout + (j * 2), "%02X", cksum[j]);
					}

					fprintf(hashfile, "file: %s, lump: %s, hash: %s, size: %d\n", filename,
						lump->FullName.IsNotEmpty() ? lump->FullName.GetChars() : lump->Name,
						cksumout, lump->LumpSize);

					delete reader;
				}
			}
		}
		return;
	}
}

//==========================================================================
//
// W_CheckIfWadLoaded
//
// Returns true if the specified wad is loaded, false otherwise.
// If a fully-qualified path is specified, then the wad must match exactly.
// Otherwise, any wad with that name will work, whatever its path.
// Returns the wads index if found, or -1 if not.
//
//==========================================================================

int FWadCollection::CheckIfWadLoaded (const char *name)
{
	unsigned int i;

	if (strrchr (name, '/') != NULL)
	{
		for (i = 0; i < Files.Size(); ++i)
		{
			if (stricmp (GetWadFullName (i), name) == 0)
			{
				return i;
			}
		}
	}
	else
	{
		for (i = 0; i < Files.Size(); ++i)
		{
			if (stricmp (GetWadName (i), name) == 0)
			{
				return i;
			}
		}
	}
	return -1;
}

//==========================================================================
//
// W_NumLumps
//
//==========================================================================

int FWadCollection::GetNumLumps () const
{
	return NumLumps;
}

//==========================================================================
//
// GetNumFiles
//
//==========================================================================

int FWadCollection::GetNumWads () const
{
	return Files.Size();
}

//==========================================================================
//
// W_CheckNumForName
//
// Returns -1 if name not found. The version with a third parameter will
// look exclusively in the specified wad for the lump.
//
// [RH] Changed to use hash lookup ala BOOM instead of a linear search
// and namespace parameter
//==========================================================================

int FWadCollection::CheckNumForName (const char *name, int space)
{
	union
	{
		char uname[8];
		QWORD qname;
	};
	DWORD i;

	if (name == NULL)
	{
		return -1;
	}

	// Let's not search for names that are longer than 8 characters and contain path separators
	// They are almost certainly full path names passed to this function.
	if (strlen(name) > 8 && strpbrk(name, "/."))
	{
		return -1;
	}

	uppercopy (uname, name);
	i = FirstLumpIndex[LumpNameHash (uname) % NumLumps];

	while (i != NULL_INDEX)
	{
		FResourceLump *lump = LumpInfo[i].lump;

		if (lump->qwName == qname)
		{
			if (lump->Namespace == space) break;
			// If the lump is from one of the special namespaces exclusive to Zips
			// the check has to be done differently:
			// If we find a lump with this name in the global namespace that does not come
			// from a Zip return that. WADs don't know these namespaces and single lumps must
			// work as well.
			if (space > ns_specialzipdirectory && lump->Namespace == ns_global && 
				!(lump->Flags & LUMPF_ZIPFILE)) break;
		}
		i = NextLumpIndex[i];
	}

	return i != NULL_INDEX ? i : -1;
}

int FWadCollection::CheckNumForName (const char *name, int space, int wadnum, bool exact)
{
	FResourceLump *lump;
	union
	{
		char uname[8];
		QWORD qname;
	};
	DWORD i;

	if (wadnum < 0)
	{
		return CheckNumForName (name, space);
	}

	uppercopy (uname, name);
	i = FirstLumpIndex[LumpNameHash (uname) % NumLumps];

	// If exact is true if will only find lumps in the same WAD, otherwise
	// also those in earlier WADs.

	while (i != NULL_INDEX &&
		(lump = LumpInfo[i].lump, lump->qwName != qname ||
		lump->Namespace != space ||
		 (exact? (LumpInfo[i].wadnum != wadnum) : (LumpInfo[i].wadnum > wadnum)) ))
	{
		i = NextLumpIndex[i];
	}

	return i != NULL_INDEX ? i : -1;
}

//==========================================================================
//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
//==========================================================================

int FWadCollection::GetNumForName (const char *name, int space)
{
	int	i;

	i = CheckNumForName (name, space);

	if (i == -1)
		I_Error ("W_GetNumForName: %s not found!", name);

	return i;
}


//==========================================================================
//
// W_CheckNumForFullName
//
// Same as above but looks for a fully qualified name from a .zip
// These don't care about namespaces though because those are part
// of the path.
//
//==========================================================================

int FWadCollection::CheckNumForFullName (const char *name, bool trynormal, int namespc)
{
	DWORD i;

	if (name == NULL)
	{
		return -1;
	}

	i = FirstLumpIndex_FullName[MakeKey(name) % NumLumps];

	while (i != NULL_INDEX && stricmp(name, LumpInfo[i].lump->FullName))
	{
		i = NextLumpIndex_FullName[i];
	}

	if (i != NULL_INDEX) return i;

	if (trynormal && strlen(name) <= 8 && !strpbrk(name, "./"))
	{
		return CheckNumForName(name, namespc);
	}
	return -1;
}

int FWadCollection::CheckNumForFullName (const char *name, int wadnum)
{
	DWORD i;

	if (wadnum < 0)
	{
		return CheckNumForFullName (name);
	}

	i = FirstLumpIndex_FullName[MakeKey (name) % NumLumps];

	while (i != NULL_INDEX && 
		(stricmp(name, LumpInfo[i].lump->FullName) || LumpInfo[i].wadnum != wadnum))
	{
		i = NextLumpIndex_FullName[i];
	}

	return i != NULL_INDEX ? i : -1;
}

//==========================================================================
//
// W_GetNumForFullName
//
// Calls W_CheckNumForFullName, but bombs out if not found.
//
//==========================================================================

int FWadCollection::GetNumForFullName (const char *name)
{
	int	i;

	i = CheckNumForFullName (name);

	if (i == -1)
		I_Error ("GetNumForFullName: %s not found!", name);

	return i;
}

//==========================================================================
//
// link a texture with a given lump
//
//==========================================================================

void FWadCollection::SetLinkedTexture(int lump, FTexture *tex)
{
	if ((size_t)lump < NumLumps)
	{
		FResourceLump *reslump = LumpInfo[lump].lump;
		reslump->LinkedTexture = tex;
	}
}

//==========================================================================
//
// retrieve linked texture
//
//==========================================================================

FTexture *FWadCollection::GetLinkedTexture(int lump)
{
	if ((size_t)lump < NumLumps)
	{
		FResourceLump *reslump = LumpInfo[lump].lump;
		return reslump->LinkedTexture;
	}
	return NULL;
}

//==========================================================================
//
// W_LumpLength
//
// Returns the buffer size needed to load the given lump.
//
//==========================================================================

int FWadCollection::LumpLength (int lump) const
{
	if ((size_t)lump >= NumLumps)
	{
		I_Error ("W_LumpLength: %i >= NumLumps",lump);
	}

	return LumpInfo[lump].lump->LumpSize;
}

//==========================================================================
//
// GetLumpOffset
//
// Returns the offset from the beginning of the file to the lump.
// Returns -1 if the lump is compressed or can't be read directly
//
//==========================================================================

int FWadCollection::GetLumpOffset (int lump)
{
	if ((size_t)lump >= NumLumps)
	{
		I_Error ("GetLumpOffset: %i >= NumLumps",lump);
	}

	return LumpInfo[lump].lump->GetFileOffset();
}

//==========================================================================
//
// GetLumpOffset
//
//==========================================================================

int FWadCollection::GetLumpFlags (int lump)
{
	if ((size_t)lump >= NumLumps)
	{
		return 0;
	}

	return LumpInfo[lump].lump->Flags;
}

//==========================================================================
//
// W_LumpNameHash
//
// NOTE: s should already be uppercase, in contrast to the BOOM version.
//
// Hash function used for lump names.
// Must be mod'ed with table size.
// Can be used for any 8-character names.
//
//==========================================================================

DWORD FWadCollection::LumpNameHash (const char *s)
{
	const DWORD *table = GetCRCTable ();;
	DWORD hash = 0xffffffff;
	int i;

	for (i = 8; i > 0 && *s; --i, ++s)
	{
		hash = CRC1 (hash, *s, table);
	}
	return hash ^ 0xffffffff;
}

//==========================================================================
//
// W_InitHashChains
//
// Prepares the lumpinfos for hashing.
// (Hey! This looks suspiciously like something from Boom! :-)
//
//==========================================================================

void FWadCollection::InitHashChains (void)
{
	char name[8];
	unsigned int i, j;

	// Mark all buckets as empty
	memset (FirstLumpIndex, 255, NumLumps*sizeof(FirstLumpIndex[0]));
	memset (NextLumpIndex, 255, NumLumps*sizeof(NextLumpIndex[0]));
	memset (FirstLumpIndex_FullName, 255, NumLumps*sizeof(FirstLumpIndex_FullName[0]));
	memset (NextLumpIndex_FullName, 255, NumLumps*sizeof(NextLumpIndex_FullName[0]));

	// Now set up the chains
	for (i = 0; i < (unsigned)NumLumps; i++)
	{
		uppercopy (name, LumpInfo[i].lump->Name);
		j = LumpNameHash (name) % NumLumps;
		NextLumpIndex[i] = FirstLumpIndex[j];
		FirstLumpIndex[j] = i;

		// Do the same for the full paths
		if (LumpInfo[i].lump->FullName.IsNotEmpty())
		{
			j = MakeKey(LumpInfo[i].lump->FullName) % NumLumps;
			NextLumpIndex_FullName[i] = FirstLumpIndex_FullName[j];
			FirstLumpIndex_FullName[j] = i;
		}
	}
}

//==========================================================================
//
// RenameSprites
//
// Renames sprites in IWADs so that unique actors can have unique sprites,
// making it possible to import any actor from any game into any other
// game without jumping through hoops to resolve duplicate sprite names.
// You just need to know what the sprite's new name is.
//
//==========================================================================

void FWadCollection::RenameSprites ()
{
	bool renameAll;
	bool MNTRZfound = false;

	static const DWORD HereticRenames[] =
	{ MAKE_ID('H','E','A','D'), MAKE_ID('L','I','C','H'),		// Ironlich
	};

	static const DWORD HexenRenames[] =
	{ MAKE_ID('B','A','R','L'), MAKE_ID('Z','B','A','R'),		// ZBarrel
	  MAKE_ID('A','R','M','1'), MAKE_ID('A','R','_','1'),		// MeshArmor
	  MAKE_ID('A','R','M','2'), MAKE_ID('A','R','_','2'),		// FalconShield
	  MAKE_ID('A','R','M','3'), MAKE_ID('A','R','_','3'),		// PlatinumHelm
	  MAKE_ID('A','R','M','4'), MAKE_ID('A','R','_','4'),		// AmuletOfWarding
	  MAKE_ID('S','U','I','T'), MAKE_ID('Z','S','U','I'),		// ZSuitOfArmor and ZArmorChunk
	  MAKE_ID('T','R','E','1'), MAKE_ID('Z','T','R','E'),		// ZTree and ZTreeDead
	  MAKE_ID('T','R','E','2'), MAKE_ID('T','R','E','S'),		// ZTreeSwamp150
	  MAKE_ID('C','A','N','D'), MAKE_ID('B','C','A','N'),		// ZBlueCandle
	  MAKE_ID('R','O','C','K'), MAKE_ID('R','O','K','K'),		// rocks and dirt in a_debris.cpp
	  MAKE_ID('W','A','T','R'), MAKE_ID('H','W','A','T'),		// Strife also has WATR
	  MAKE_ID('G','I','B','S'), MAKE_ID('P','O','L','5'),		// RealGibs
	  MAKE_ID('E','G','G','M'), MAKE_ID('P','R','K','M'),		// PorkFX
	  MAKE_ID('I','N','V','U'), MAKE_ID('D','E','F','N'),		// Icon of the Defender
	};

	static const DWORD StrifeRenames[] =
	{ MAKE_ID('M','I','S','L'), MAKE_ID('S','M','I','S'),		// lots of places
	  MAKE_ID('A','R','M','1'), MAKE_ID('A','R','M','3'),		// MetalArmor
	  MAKE_ID('A','R','M','2'), MAKE_ID('A','R','M','4'),		// LeatherArmor
	  MAKE_ID('P','M','A','P'), MAKE_ID('S','M','A','P'),		// StrifeMap
	  MAKE_ID('T','L','M','P'), MAKE_ID('T','E','C','H'),		// TechLampSilver and TechLampBrass
	  MAKE_ID('T','R','E','1'), MAKE_ID('T','R','E','T'),		// TreeStub
	  MAKE_ID('B','A','R','1'), MAKE_ID('B','A','R','C'),		// BarricadeColumn
	  MAKE_ID('S','H','T','2'), MAKE_ID('M','P','U','F'),		// MaulerPuff
	  MAKE_ID('B','A','R','L'), MAKE_ID('B','B','A','R'),		// StrifeBurningBarrel
	  MAKE_ID('T','R','C','H'), MAKE_ID('T','R','H','L'),		// SmallTorchLit
	  MAKE_ID('S','H','R','D'), MAKE_ID('S','H','A','R'),		// glass shards
	  MAKE_ID('B','L','S','T'), MAKE_ID('M','A','U','L'),		// Mauler
	  MAKE_ID('L','O','G','G'), MAKE_ID('L','O','G','W'),		// StickInWater
	  MAKE_ID('V','A','S','E'), MAKE_ID('V','A','Z','E'),		// Pot and Pitcher
	  MAKE_ID('C','N','D','L'), MAKE_ID('K','N','D','L'),		// Candle
	  MAKE_ID('P','O','T','1'), MAKE_ID('M','P','O','T'),		// MetalPot
	  MAKE_ID('S','P','I','D'), MAKE_ID('S','T','L','K'),		// Stalker
	};

	const DWORD *renames;
	int numrenames;

	switch (gameinfo.gametype)
	{
	case GAME_Doom:
	default:
		// Doom's sprites don't get renamed.
		return;

	case GAME_Heretic:
		renames = HereticRenames;
		numrenames = sizeof(HereticRenames)/8;
		break;

	case GAME_Hexen:
		renames = HexenRenames;
		numrenames = sizeof(HexenRenames)/8;
		break;

	case GAME_Strife:
		renames = StrifeRenames;
		numrenames = sizeof(StrifeRenames)/8;
		break;
	}


	for (DWORD i=0; i< LumpInfo.Size(); i++)
	{
		// check for full Minotaur animations. If this is not found
		// some frames need to be renamed.
		if (LumpInfo[i].lump->Namespace == ns_sprites)
		{
			if (LumpInfo[i].lump->dwName == MAKE_ID('M', 'N', 'T', 'R') && LumpInfo[i].lump->Name[4] == 'Z' )
			{
				MNTRZfound = true;
				break;
			}
		}
	}

	renameAll = !!Args->CheckParm ("-oldsprites") || nospriterename;
	
	for (DWORD i = 0; i < LumpInfo.Size(); i++)
	{
		if (LumpInfo[i].lump->Namespace == ns_sprites)
		{
			// Only sprites in the IWAD normally get renamed
			if (renameAll || LumpInfo[i].wadnum == IWAD_FILENUM)
			{
				for (int j = 0; j < numrenames; ++j)
				{
					if (LumpInfo[i].lump->dwName == renames[j*2])
					{
						LumpInfo[i].lump->dwName = renames[j*2+1];
					}
				}
				if (gameinfo.gametype == GAME_Hexen)
				{
					if (CheckLumpName (i, "ARTIINVU"))
					{
						LumpInfo[i].lump->Name[4]='D'; LumpInfo[i].lump->Name[5]='E';
						LumpInfo[i].lump->Name[6]='F'; LumpInfo[i].lump->Name[7]='N';
					}
				}
			}

			if (!MNTRZfound)
			{
				if (LumpInfo[i].lump->dwName == MAKE_ID('M', 'N', 'T', 'R'))
				{
					if (LumpInfo[i].lump->Name[4] >= 'F' && LumpInfo[i].lump->Name[4] <= 'K')
					{
						LumpInfo[i].lump->Name[4] += 'U' - 'F';
					}
				}
			}
			
			// When not playing Doom rename all BLOD sprites to BLUD so that
			// the same blood states can be used everywhere
			if (!(gameinfo.gametype & GAME_DoomChex))
			{
				if (LumpInfo[i].lump->dwName == MAKE_ID('B', 'L', 'O', 'D'))
				{
					LumpInfo[i].lump->dwName = MAKE_ID('B', 'L', 'U', 'D');
				}
			}
		}
	}
}

//==========================================================================
//
// RenameNerve
//
// Renames map headers and map name pictures in nerve.wad so as to load it
// alongside Doom II and offer both episodes without causing conflicts.
// MD5 checksum for NERVE.WAD: 967d5ae23daf45196212ae1b605da3b0
//
//==========================================================================
void FWadCollection::RenameNerve ()
{
	if (gameinfo.gametype != GAME_Doom)
		return;

	bool found = false;
	BYTE cksum[16];
	static const BYTE nerve[16] = { 0x96, 0x7d, 0x5a, 0xe2, 0x3d, 0xaf, 0x45, 0x19,
		0x62, 0x12, 0xae, 0x1b, 0x60, 0x5d, 0xa3, 0xb0 };
	size_t nervesize = 3819855; // NERVE.WAD's file size
	int w = IWAD_FILENUM;
	while (++w < GetNumWads())
	{
		FileReader *fr = GetFileReader(w);
		if (fr == NULL)
		{
			continue;
		}
		if (fr->GetLength() != (long)nervesize)
		{
			// Skip MD5 computation when there is a
			// cheaper way to know this is not the file
			continue;
		}
		fr->Seek(0, SEEK_SET);
		MD5Context md5;
		md5.Update(fr, fr->GetLength());
		md5.Final(cksum);
		if (memcmp(nerve, cksum, 16) == 0)
		{
			found = true;
			break;
		}
	}

	if (!found)
		return;

	for (int i = GetFirstLump(w); i <= GetLastLump(w); i++)
	{
		// Only rename the maps from NERVE.WAD
		assert(LumpInfo[i].wadnum == w);
		if (LumpInfo[i].lump->dwName == MAKE_ID('C', 'W', 'I', 'L'))
		{
			LumpInfo[i].lump->Name[0] = 'N';
		}
		else if (LumpInfo[i].lump->dwName == MAKE_ID('M', 'A', 'P', '0'))
		{
			LumpInfo[i].lump->Name[6] = LumpInfo[i].lump->Name[4];
			LumpInfo[i].lump->Name[5] = '0';
			LumpInfo[i].lump->Name[4] = 'L';
			LumpInfo[i].lump->dwName = MAKE_ID('L', 'E', 'V', 'E');
		}
	}
}

//==========================================================================
//
// W_FindLump
//
// Find a named lump. Specifically allows duplicates for merging of e.g.
// SNDINFO lumps.
//
//==========================================================================

int FWadCollection::FindLump (const char *name, int *lastlump, bool anyns)
{
	union
	{
		char name8[8];
		QWORD qname;
	};
	LumpRecord *lump_p;

	uppercopy (name8, name);

	assert(lastlump != NULL && *lastlump >= 0);
	lump_p = &LumpInfo[*lastlump];
	while (lump_p < &LumpInfo[NumLumps])
	{
		FResourceLump *lump = lump_p->lump;

		if ((anyns || lump->Namespace == ns_global) && lump->qwName == qname)
		{
			int lump = int(lump_p - &LumpInfo[0]);
			*lastlump = lump + 1;
			return lump;
		}
		lump_p++;
	}

	*lastlump = NumLumps;
	return -1;
}

//==========================================================================
//
// W_FindLumpMulti
//
// Find a named lump. Specifically allows duplicates for merging of e.g.
// SNDINFO lumps. Returns everything having one of the passed names.
//
//==========================================================================

int FWadCollection::FindLumpMulti (const char **names, int *lastlump, bool anyns, int *nameindex)
{
	LumpRecord *lump_p;

	assert(lastlump != NULL && *lastlump >= 0);
	lump_p = &LumpInfo[*lastlump];
	while (lump_p < &LumpInfo[NumLumps])
	{
		FResourceLump *lump = lump_p->lump;

		if (anyns || lump->Namespace == ns_global)
		{
			
			for(const char **name = names; *name != NULL; name++)
			{
				if (!strnicmp(*name, lump->Name, 8))
				{
					int lump = int(lump_p - &LumpInfo[0]);
					*lastlump = lump + 1;
					if (nameindex != NULL) *nameindex = int(name - names);
					return lump;
				}
			}
		}
		lump_p++;
	}

	*lastlump = NumLumps;
	return -1;
}

//==========================================================================
//
// W_CheckLumpName
//
//==========================================================================

bool FWadCollection::CheckLumpName (int lump, const char *name)
{
	if ((size_t)lump >= NumLumps)
		return false;

	return !strnicmp (LumpInfo[lump].lump->Name, name, 8);
}

//==========================================================================
//
// W_GetLumpName
//
//==========================================================================

void FWadCollection::GetLumpName (char *to, int lump) const
{
	if ((size_t)lump >= NumLumps)
		*to = 0;
	else
		uppercopy (to, LumpInfo[lump].lump->Name);
}

void FWadCollection::GetLumpName(FString &to, int lump) const
{
	if ((size_t)lump >= NumLumps)
		to = FString();
	else {
		to = LumpInfo[lump].lump->Name;
		to.ToUpper();
	}
}

//==========================================================================
//
// FWadCollection :: GetLumpFullName
//
// Returns the lump's full name if it has one or its short name if not.
//
//==========================================================================

const char *FWadCollection::GetLumpFullName (int lump) const
{
	if ((size_t)lump >= NumLumps)
		return NULL;
	else if (LumpInfo[lump].lump->FullName.IsNotEmpty())
		return LumpInfo[lump].lump->FullName;
	else
		return LumpInfo[lump].lump->Name;
}

//==========================================================================
//
// FWadCollection :: GetLumpFullPath
//
// Returns the name of the lump's wad prefixed to the lump's full name.
//
//==========================================================================

FString FWadCollection::GetLumpFullPath(int lump) const
{
	FString foo;

	if ((size_t) lump <  NumLumps)
	{
		foo << GetWadName(LumpInfo[lump].wadnum) << ':' << GetLumpFullName(lump);
	}
	return foo;
}

//==========================================================================
//
// GetLumpNamespace
//
//==========================================================================

int FWadCollection::GetLumpNamespace (int lump) const
{
	if ((size_t)lump >= NumLumps)
		return ns_global;
	else
		return LumpInfo[lump].lump->Namespace;
}

//==========================================================================
//
// FWadCollection :: GetLumpIndexNum
//
// Returns the index number for this lump. This is *not* the lump's position
// in the lump directory, but rather a special value that RFF can associate
// with files. Other archive types will return 0, since they don't have it.
//
//==========================================================================

int FWadCollection::GetLumpIndexNum(int lump) const
{
	if ((size_t)lump >= NumLumps)
		return 0;
	else
		return LumpInfo[lump].lump->GetIndexNum();
}

//==========================================================================
//
// W_GetLumpFile
//
//==========================================================================

int FWadCollection::GetLumpFile (int lump) const
{
	if ((size_t)lump >= LumpInfo.Size())
		return -1;
	return LumpInfo[lump].wadnum;
}

//==========================================================================
//
// W_ReadLump
//
// Loads the lump into the given buffer, which must be >= W_LumpLength().
//
//==========================================================================

void FWadCollection::ReadLump (int lump, void *dest)
{
	FWadLump lumpr = OpenLumpNum (lump);
	long size = lumpr.GetLength ();
	long numread = lumpr.Read (dest, size);

	if (numread != size)
	{
		I_Error ("W_ReadLump: only read %ld of %ld on lump %i\n",
			numread, size, lump);	
	}
}

//==========================================================================
//
// ReadLump - variant 2
//
// Loads the lump into a newly created buffer and returns it.
//
//==========================================================================

FMemLump FWadCollection::ReadLump (int lump)
{
	return FMemLump(FString(ELumpNum(lump)));
}

//==========================================================================
//
// OpenLumpNum
//
// Returns a copy of the file object for a lump's wad and positions its
// file pointer at the beginning of the lump.
//
//==========================================================================

FWadLump FWadCollection::OpenLumpNum (int lump)
{
	if ((unsigned)lump >= (unsigned)LumpInfo.Size())
	{
		I_Error ("W_OpenLumpNum: %u >= NumLumps", lump);
	}

	return FWadLump(LumpInfo[lump].lump);
}

//==========================================================================
//
// ReopenLumpNum
//
// Similar to OpenLumpNum, except a new, independant file object is created
// for the lump's wad. Use this when you won't read the lump's data all at
// once (e.g. for streaming an Ogg Vorbis stream from a wad as music).
//
//==========================================================================

FWadLump *FWadCollection::ReopenLumpNum (int lump)
{
	if ((unsigned)lump >= (unsigned)LumpInfo.Size())
	{
		I_Error ("W_ReopenLumpNum: %u >= NumLumps", lump);
	}

	return new FWadLump(LumpInfo[lump].lump, true);
}

FWadLump *FWadCollection::ReopenLumpNumNewFile (int lump)
{
	if ((unsigned)lump >= (unsigned)LumpInfo.Size())
	{
		return NULL;
	}

	return new FWadLump(lump, LumpInfo[lump].lump);
}


//==========================================================================
//
// GetFileReader
//
// Retrieves the FileReader object to access the given WAD
// Careful: This is only useful for real WAD files!
//
//==========================================================================

FileReader *FWadCollection::GetFileReader(int wadnum)
{
	if ((DWORD)wadnum >= Files.Size())
	{
		return NULL;
	}
	return Files[wadnum]->GetReader();
}

//==========================================================================
//
// W_GetWadName
//
// Returns the name of the given wad.
//
//==========================================================================

const char *FWadCollection::GetWadName (int wadnum) const
{
	const char *name, *slash;

	if ((DWORD)wadnum >= Files.Size())
	{
		return NULL;
	}

	name = Files[wadnum]->Filename;
	slash = strrchr (name, '/');
	return slash != NULL ? slash+1 : name;
}

//==========================================================================
//
//
//==========================================================================

int FWadCollection::GetFirstLump (int wadnum) const
{
	if ((DWORD)wadnum >= Files.Size())
	{
		return 0;
	}

	return Files[wadnum]->GetFirstLump();
}

//==========================================================================
//
//
//==========================================================================

int FWadCollection::GetLastLump (int wadnum) const
{
	if ((DWORD)wadnum >= Files.Size())
	{
		return 0;
	}

	return Files[wadnum]->GetFirstLump() + Files[wadnum]->LumpCount() - 1;
}

//==========================================================================
//
// W_GetWadFullName
//
// Returns the name of the given wad, including any path
//
//==========================================================================

const char *FWadCollection::GetWadFullName (int wadnum) const
{
	if ((unsigned int)wadnum >= Files.Size())
	{
		return NULL;
	}

	return Files[wadnum]->Filename;
}


//==========================================================================
//
// IsUncompressedFile
//
// Returns true when the lump is available as an uncompressed portion of
// a file. The music player can play such lumps by streaming but anything
// else has to be loaded into memory first.
//
//==========================================================================

bool FWadCollection::IsUncompressedFile(int lump) const
{
	if ((unsigned)lump >= (unsigned)NumLumps)
	{
		I_Error ("IsUncompressedFile: %u >= NumLumps",lump);
	}

	FResourceLump *l = LumpInfo[lump].lump;
	FileReader *f = l->GetReader();
	
	// We can access the file only if we get the FILE pointer from the FileReader here.
	// Any other case means it won't work.
	return (f != NULL && f->GetFile() != NULL);
}

//==========================================================================
//
// IsEncryptedFile
//
// Returns true if the first 256 bytes of the lump are encrypted for Blood.
//
//==========================================================================

bool FWadCollection::IsEncryptedFile(int lump) const
{
	if ((unsigned)lump >= (unsigned)NumLumps)
	{
		return false;
	}
	return !!(LumpInfo[lump].lump->Flags & LUMPF_BLOODCRYPT);
}


// FWadLump -----------------------------------------------------------------

FWadLump::FWadLump ()
: FileReader(), Lump(NULL)
{
}

FWadLump::FWadLump (const FWadLump &copy)
: FileReader()
{
	// This must be defined isn't called.
	File = copy.File;
	Length = copy.Length;
	FilePos = copy.FilePos;
	StartPos = copy.StartPos;
	CloseOnDestruct = false;
	if ((Lump = copy.Lump)) Lump->CacheLump();
}

#ifdef _DEBUG
FWadLump & FWadLump::operator= (const FWadLump &copy)
{
	// Only the debug build actually calls this!
	File = copy.File;
	Length = copy.Length;
	FilePos = copy.FilePos;
	StartPos = copy.StartPos;
	CloseOnDestruct = false;
	if ((Lump = copy.Lump)) Lump->CacheLump();
	return *this;
}
#endif


FWadLump::FWadLump(FResourceLump *lump, bool alwayscache)
: FileReader()
{
	FileReader *f = lump->GetReader();

	if (f != NULL && f->GetFile() != NULL && !alwayscache)
	{
		// Uncompressed lump in a file
		File = f->GetFile();
		Length = lump->LumpSize;
		StartPos = FilePos = lump->GetFileOffset();
		Lump = NULL;
	}
	else
	{
		File = NULL;
		Length = lump->LumpSize;
		StartPos = FilePos = 0;
		Lump = lump;
		Lump->CacheLump();
	}
}

FWadLump::FWadLump(int lumpnum, FResourceLump *lump)
: FileReader()
{
	FileReader *f = lump->GetReader();

	if (f != NULL && f->GetFile() != NULL)
	{
		// Uncompressed lump in a file. For this we will have to open a new FILE, since we need it for streaming
		int fileno = Wads.GetLumpFile(lumpnum);
		const char *filename = Wads.GetWadFullName(fileno);
		File = fopen(filename, "rb");
		if (File != NULL)
		{
			Length = lump->LumpSize;
			StartPos = FilePos = lump->GetFileOffset();
			Lump = NULL;
			CloseOnDestruct = true;
			Seek(0, SEEK_SET);
			return;
		}
	}
	File = NULL;
	Length = lump->LumpSize;
	StartPos = FilePos = 0;
	Lump = lump;
	Lump->CacheLump();
}

FWadLump::~FWadLump()
{
	if (Lump != NULL)
	{
		Lump->ReleaseCache();
	}
}

long FWadLump::Seek (long offset, int origin)
{
	if (Lump != NULL)
	{
		switch (origin)
		{
		case SEEK_CUR:
			offset += FilePos;
			break;

		case SEEK_END:
			offset += Length;
			break;

		default:
			break;
		}
		FilePos = clamp<long> (offset, 0, Length);
		return 0;
	}
	return FileReader::Seek(offset, origin);
}

long FWadLump::Read (void *buffer, long len)
{
	long numread;
	long startread = FilePos;

	if (Lump != NULL)
	{
		if (FilePos + len > Length)
		{
			len = Length - FilePos;
		}
		memcpy(buffer, Lump->Cache + FilePos, len);
		FilePos += len;
		numread = len;
	}
	else
	{
		numread = FileReader::Read(buffer, len);
	}
	return numread;
}

char *FWadLump::Gets(char *strbuf, int len)
{
	if (Lump != NULL)
	{
		return GetsFromBuffer(Lump->Cache, strbuf, len);
	}
	else
	{
		return FileReader::Gets(strbuf, len);
	}
	return strbuf;
}

// FMemLump -----------------------------------------------------------------

FMemLump::FMemLump ()
{
}

FMemLump::FMemLump (const FMemLump &copy)
{
	Block = copy.Block;
}

FMemLump &FMemLump::operator = (const FMemLump &copy)
{
	Block = copy.Block;
	return *this;
}

FMemLump::FMemLump (const FString &source)
: Block (source)
{
}

FMemLump::~FMemLump ()
{
}

FString::FString (ELumpNum lumpnum)
{
	FWadLump lumpr = Wads.OpenLumpNum ((int)lumpnum);
	long size = lumpr.GetLength ();
	AllocBuffer (1 + size);
	long numread = lumpr.Read (&Chars[0], size);
	Chars[size] = '\0';

	if (numread != size)
	{
		I_Error ("ConstructStringFromLump: Only read %ld of %ld bytes on lump %i (%s)\n",
			numread, size, lumpnum, Wads.GetLumpFullName((int)lumpnum));
	}
}

//==========================================================================
//
// PrintLastError
//
//==========================================================================

#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>

extern "C" {
__declspec(dllimport) unsigned long __stdcall FormatMessageA(
    unsigned long dwFlags,
    const void *lpSource,
    unsigned long dwMessageId,
    unsigned long dwLanguageId,
    char **lpBuffer,
    unsigned long nSize,
    va_list *Arguments
    );
__declspec(dllimport) void * __stdcall LocalFree (void *);
__declspec(dllimport) unsigned long __stdcall GetLastError ();
}

static void PrintLastError ()
{
	char *lpMsgBuf;
	FormatMessageA(0x1300 /*FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM | 
							FORMAT_MESSAGE_IGNORE_INSERTS*/,
		NULL,
		GetLastError(),
		1 << 10 /*MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)*/, // Default language
		&lpMsgBuf,
		0,
		NULL 
	);
	Printf (TEXTCOLOR_RED "  %s\n", lpMsgBuf);
	// Free the buffer.
	LocalFree( lpMsgBuf );
}
#else
static void PrintLastError ()
{
	Printf (TEXTCOLOR_RED "  %s\n", strerror(errno));
}
#endif

#ifdef _DEBUG
//==========================================================================
//
// CCMD LumpNum
//
//==========================================================================

CCMD(lumpnum)
{
	for (int i = 1; i < argv.argc(); ++i)
	{
		Printf("%s: %d\n", argv[i], Wads.CheckNumForName(argv[i]));
	}
}

//==========================================================================
//
// CCMD LumpNumFull
//
//==========================================================================

CCMD(lumpnumfull)
{
	for (int i = 1; i < argv.argc(); ++i)
	{
		Printf("%s: %d\n", argv[i], Wads.CheckNumForFullName(argv[i]));
	}
}
#endif
