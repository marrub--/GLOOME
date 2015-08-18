/*
** multipatchtexture.cpp
** Texture class for standard Doom multipatch textures
**
**---------------------------------------------------------------------------
** Copyright 2004-2006 Randy Heit
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

#include <ctype.h>
#include "doomtype.h"
#include "files.h"
#include "w_wad.h"
#include "i_system.h"
#include "gi.h"
#include "st_start.h"
#include "sc_man.h"
#include "templates.h"
#include "r_data/r_translate.h"
#include "bitmap.h"
#include "colormatcher.h"
#include "v_palette.h"
#include "v_video.h"
#include "v_text.h"
#include "m_fixed.h"
#include "textures/textures.h"
#include "r_data/colormaps.h"

// On the Alpha, accessing the shorts directly if they aren't aligned on a
// 4-byte boundary causes unaligned access warnings. Why it does this at
// all and only while initing the textures is beyond me.

#ifdef ALPHA
#define SAFESHORT(s)	((short)(((BYTE *)&(s))[0] + ((BYTE *)&(s))[1] * 256))
#else
#define SAFESHORT(s)	LittleShort(s)
#endif



//--------------------------------------------------------------------------
//
// Data structures for the TEXTUREx lumps
//
//--------------------------------------------------------------------------

//
// Each texture is composed of one or more patches, with patches being lumps
// stored in the WAD. The lumps are referenced by number, and patched into
// the rectangular texture space using origin and possibly other attributes.
//
struct mappatch_t
{
	SWORD	originx;
	SWORD	originy;
	SWORD	patch;
	SWORD	stepdir;
	SWORD	colormap;
};

//
// A wall texture is a list of patches which are to be combined in a
// predefined order.
//
struct maptexture_t
{
	BYTE		name[8];
	WORD		Flags;				// [RH] Was unused
	BYTE		ScaleX;				// [RH] Scaling (8 is normal)
	BYTE		ScaleY;				// [RH] Same as above
	SWORD		width;
	SWORD		height;
	BYTE		columndirectory[4];	// OBSOLETE
	SWORD		patchcount;
	mappatch_t	patches[1];
};

#define MAPTEXF_WORLDPANNING	0x8000

// Strife uses versions of the above structures that remove all unused fields

struct strifemappatch_t
{
	SWORD	originx;
	SWORD	originy;
	SWORD	patch;
};

//
// A wall texture is a list of patches which are to be combined in a
// predefined order.
//
struct strifemaptexture_t
{
	BYTE		name[8];
	WORD		Flags;				// [RH] Was unused
	BYTE		ScaleX;				// [RH] Scaling (8 is normal)
	BYTE		ScaleY;				// [RH] Same as above
	SWORD		width;
	SWORD		height;
	SWORD		patchcount;
	strifemappatch_t	patches[1];
};


//==========================================================================
//
// In-memory representation of a single PNAMES lump entry
//
//==========================================================================

struct FPatchLookup
{
	FString Name;
	FTexture *Texture;
};


//==========================================================================
//
// A texture defined in a TEXTURE1 or TEXTURE2 lump
//
//==========================================================================

class FMultiPatchTexture : public FTexture
{
public:
	FMultiPatchTexture (const void *texdef, FPatchLookup *patchlookup, int maxpatchnum, bool strife, int deflump);
	FMultiPatchTexture (FScanner &sc, int usetype);
	~FMultiPatchTexture ();

	const BYTE *GetColumn (unsigned int column, const Span **spans_out);
	const BYTE *GetPixels ();
	FTextureFormat GetFormat();
	bool UseBasePalette() ;
	void Unload ();
	virtual void SetFrontSkyLayer ();

	int CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate, FCopyInfo *inf = NULL);
	int GetSourceLump() { return DefinitionLump; }
	FTexture *GetRedirect(bool wantwarped);
	FTexture *GetRawTexture();

protected:
	BYTE *Pixels;
	Span **Spans;
	int DefinitionLump;

	struct TexPart
	{
		SWORD OriginX, OriginY;
		BYTE Rotate;
		BYTE op;
		FRemapTable *Translation;
		PalEntry Blend;
		FTexture *Texture;
		fixed_t Alpha;

		TexPart();
	};

	int NumParts;
	TexPart *Parts;
	bool bRedirect:1;
	bool bTranslucentPatches:1;

	void MakeTexture ();

private:
	void CheckForHacks ();
	void ParsePatch(FScanner &sc, TexPart & part, bool silent, int usetype);
};

//==========================================================================
//
// FMultiPatchTexture :: FMultiPatchTexture
//
//==========================================================================

FMultiPatchTexture::FMultiPatchTexture (const void *texdef, FPatchLookup *patchlookup, int maxpatchnum, bool strife, int deflumpnum)
: Pixels (0), Spans(0), Parts(0), bRedirect(false), bTranslucentPatches(false)
{
	union
	{
		const maptexture_t			*d;
		const strifemaptexture_t	*s;
	}
	mtexture;

	union
	{
		const mappatch_t			*d;
		const strifemappatch_t		*s;
	}
	mpatch;

	int i;

	mtexture.d = (const maptexture_t *)texdef;
	bMultiPatch = true;

	if (strife)
	{
		NumParts = SAFESHORT(mtexture.s->patchcount);
	}
	else
	{
		NumParts = SAFESHORT(mtexture.d->patchcount);
	}

	if (NumParts < 0)
	{
		I_FatalError ("Bad texture directory");
	}

	UseType = FTexture::TEX_Wall;
	Parts = NumParts > 0 ? new TexPart[NumParts] : NULL;
	Width = SAFESHORT(mtexture.d->width);
	Height = SAFESHORT(mtexture.d->height);
	Name = (char *)mtexture.d->name;
	CalcBitSize ();

	xScale = mtexture.d->ScaleX ? mtexture.d->ScaleX*(FRACUNIT/8) : FRACUNIT;
	yScale = mtexture.d->ScaleY ? mtexture.d->ScaleY*(FRACUNIT/8) : FRACUNIT;

	if (mtexture.d->Flags & MAPTEXF_WORLDPANNING)
	{
		bWorldPanning = true;
	}

	if (strife)
	{
		mpatch.s = &mtexture.s->patches[0];
	}
	else
	{
		mpatch.d = &mtexture.d->patches[0];
	}

	for (i = 0; i < NumParts; ++i)
	{
		if (unsigned(LittleShort(mpatch.d->patch)) >= unsigned(maxpatchnum))
		{
			I_FatalError ("Bad PNAMES and/or texture directory:\n\nPNAMES has %d entries, but\n%s wants to use entry %d.",
				maxpatchnum, Name.GetChars(), LittleShort(mpatch.d->patch)+1);
		}
		Parts[i].OriginX = LittleShort(mpatch.d->originx);
		Parts[i].OriginY = LittleShort(mpatch.d->originy);
		Parts[i].Texture = patchlookup[LittleShort(mpatch.d->patch)].Texture;
		if (Parts[i].Texture == NULL)
		{
			Printf(TEXTCOLOR_RED "Unknown patch %s in texture %s\n", patchlookup[LittleShort(mpatch.d->patch)].Name.GetChars(), Name.GetChars());
			NumParts--;
			i--;
		}
		else
		{
			Parts[i].Texture->bKeepAround = true;
		}
		if (strife)
			mpatch.s++;
		else
			mpatch.d++;
	}
	if (NumParts == 0)
	{
		Printf ("Texture %s is left without any patches\n", Name.GetChars());
	}

	CheckForHacks ();

	// If this texture is just a wrapper around a single patch, we can simply
	// forward GetPixels() and GetColumn() calls to that patch.
	if (NumParts == 1)
	{
		if (Parts->OriginX == 0 && Parts->OriginY == 0 &&
			Parts->Texture->GetWidth() == Width &&
			Parts->Texture->GetHeight() == Height)
		{
			bRedirect = true;
		}
	}
	DefinitionLump = deflumpnum;
}

//==========================================================================
//
// FMultiPatchTexture :: ~FMultiPatchTexture
//
//==========================================================================

FMultiPatchTexture::~FMultiPatchTexture ()
{
	Unload ();
	if (Parts != NULL)
	{
		for(int i=0; i<NumParts;i++)
		{
			if (Parts[i].Translation != NULL) delete Parts[i].Translation;
		}
		delete[] Parts;
		Parts = NULL;
	}
	if (Spans != NULL)
	{
		FreeSpans (Spans);
		Spans = NULL;
	}
}

//==========================================================================
//
// FMultiPatchTexture :: SetFrontSkyLayer
//
//==========================================================================

void FMultiPatchTexture::SetFrontSkyLayer ()
{
	for (int i = 0; i < NumParts; ++i)
	{
		Parts[i].Texture->SetFrontSkyLayer ();
	}
	bNoRemap0 = true;
}

//==========================================================================
//
// FMultiPatchTexture :: Unload
//
//==========================================================================

void FMultiPatchTexture::Unload ()
{
	if (Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
}

//==========================================================================
//
// FMultiPatchTexture :: GetPixels
//
//==========================================================================

const BYTE *FMultiPatchTexture::GetPixels ()
{
	if (bRedirect)
	{
		return Parts->Texture->GetPixels ();
	}
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	return Pixels;
}

//==========================================================================
//
// FMultiPatchTexture :: GetColumn
//
//==========================================================================

const BYTE *FMultiPatchTexture::GetColumn (unsigned int column, const Span **spans_out)
{
	if (bRedirect)
	{
		return Parts->Texture->GetColumn (column, spans_out);
	}
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	if ((unsigned)column >= (unsigned)Width)
	{
		if (WidthMask + 1 == Width)
		{
			column &= WidthMask;
		}
		else
		{
			column %= Width;
		}
	}
	if (spans_out != NULL)
	{
		if (Spans == NULL)
		{
			Spans = CreateSpans (Pixels);
		}
		*spans_out = Spans[column];
	}
	return Pixels + column*Height;
}


//==========================================================================
//
// GetBlendMap
//
//==========================================================================

BYTE *GetBlendMap(PalEntry blend, BYTE *blendwork)
{

	switch (blend.a==0 ? int(blend) : -1)
	{
	case BLEND_ICEMAP:
		return TranslationToTable(TRANSLATION(TRANSLATION_Standard, 7))->Remap;

	default:
		if (blend >= BLEND_SPECIALCOLORMAP1 && blend < BLEND_SPECIALCOLORMAP1 + SpecialColormaps.Size())
		{
			return SpecialColormaps[blend - BLEND_SPECIALCOLORMAP1].Colormap;
		}
		else if (blend >= BLEND_DESATURATE1 && blend <= BLEND_DESATURATE31)
		{
			return DesaturateColormap[blend - BLEND_DESATURATE1];
		}
		else 
		{
			blendwork[0]=0;
			if (blend.a == 255)
			{
				for(int i=1;i<256;i++)
				{
					int rr = (blend.r * GPalette.BaseColors[i].r) / 255;
					int gg = (blend.g * GPalette.BaseColors[i].g) / 255;
					int bb = (blend.b * GPalette.BaseColors[i].b) / 255;

					blendwork[i] = ColorMatcher.Pick(rr, gg, bb);
				}
				return blendwork;
			}
			else if (blend.a != 0)
			{
				for(int i=1;i<256;i++)
				{
					int rr = (blend.r * blend.a + GPalette.BaseColors[i].r * (255-blend.a)) / 255;
					int gg = (blend.g * blend.a + GPalette.BaseColors[i].g * (255-blend.a)) / 255;
					int bb = (blend.b * blend.a + GPalette.BaseColors[i].b * (255-blend.a)) / 255;

					blendwork[i] = ColorMatcher.Pick(rr, gg, bb);
				}
				return blendwork;
			}
		}
	}
	return NULL;
}

//==========================================================================
//
// FMultiPatchTexture :: MakeTexture
//
//==========================================================================

void FMultiPatchTexture::MakeTexture ()
{
	// Add a little extra space at the end if the texture's height is not
	// a power of 2, in case somebody accidentally makes it repeat vertically.
	int numpix = Width * Height + (1 << HeightBits) - Height;
	BYTE blendwork[256];
	bool hasTranslucent = false;

	Pixels = new BYTE[numpix];
	memset (Pixels, 0, numpix);

	for (int i = 0; i < NumParts; ++i)
	{
		if (Parts[i].op != OP_COPY)
		{
			hasTranslucent = true;
		}
	}

	if (!hasTranslucent)
	{
		for (int i = 0; i < NumParts; ++i)
		{
			if (Parts[i].Texture->bHasCanvas) continue;	// cannot use camera textures as patch.
		
			BYTE *trans = Parts[i].Translation ? Parts[i].Translation->Remap : NULL;
			{
				if (Parts[i].Blend != 0)
				{
					trans = GetBlendMap(Parts[i].Blend, blendwork);
				}
				Parts[i].Texture->CopyToBlock (Pixels, Width, Height,
					Parts[i].OriginX, Parts[i].OriginY, Parts[i].Rotate, trans);
			}
		}
	}
	else
	{
		// In case there are translucent patches let's do the composition in
		// True color to keep as much precision as possible before downconverting to the palette.
		BYTE *buffer = new BYTE[Width * Height * 4];
		memset(buffer, 0, Width * Height * 4);
		FillBuffer(buffer, Width * 4, Height, TEX_RGB);
		for(int y = 0; y < Height; y++)
		{
			BYTE *in = buffer + Width * y * 4;
			BYTE *out = Pixels + y;
			for (int x = 0; x < Width; x++)
			{
				if (*out == 0 && in[3] != 0)
				{
					*out = RGB32k.RGB[in[2]>>3][in[1]>>3][in[0]>>3];
				}
				out += Height;
				in += 4;
			}
		}
		delete [] buffer;
	}
}

//===========================================================================
//
// FMultipatchTexture::CopyTrueColorPixels
//
// Preserves the palettes of each individual patch
//
//===========================================================================

int FMultiPatchTexture::CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate, FCopyInfo *inf)
{
	int retv = -1;

	if (bRedirect)
	{ // Redirect straight to the real texture's routine.
		return Parts[0].Texture->CopyTrueColorPixels(bmp, x, y, rotate, inf);
	}

	if (rotate != 0 || (inf != NULL && ((inf->op != OP_OVERWRITE && inf->op != OP_COPY) || inf->blend != BLEND_NONE)))
	{ // We are doing some sort of fancy stuff to the destination bitmap, so composite to
	  // a temporary bitmap, and copy that.
		FBitmap tbmp;
		if (tbmp.Create(Width, Height))
		{
			retv = MAX(retv, CopyTrueColorPixels(&tbmp, 0, 0, 0));
			bmp->CopyPixelDataRGB(x, y, tbmp.GetPixels(), Width, Height,
				4, tbmp.GetPitch(), rotate, CF_BGRA, inf);
		}
		return retv;
	}

	// When compositing a multipatch texture with multipatch parts,
	// drawing must be restricted to the actual area which is covered by this texture.
	FClipRect saved_cr = bmp->GetClipRect();
	bmp->IntersectClipRect(x, y, Width, Height);

	if (inf != NULL && inf->op == OP_OVERWRITE)
	{
		bmp->Zero();
	}

	for(int i = 0; i < NumParts; i++)
	{
		int ret = -1;
		FCopyInfo info;

		if (Parts[i].Texture->bHasCanvas) continue;	// cannot use camera textures as patch.

		memset (&info, 0, sizeof(info));
		info.alpha = Parts[i].Alpha;
		info.invalpha = FRACUNIT - info.alpha;
		info.op = ECopyOp(Parts[i].op);
		PalEntry b = Parts[i].Blend;
		if (b.a == 0 && b != BLEND_NONE)
		{
			info.blend = EBlend(b.d);
		}
		else if (b.a != 0)
		{
			if (b.a == 255)
			{
				info.blendcolor[0] = b.r * FRACUNIT / 255;
				info.blendcolor[1] = b.g * FRACUNIT / 255;
				info.blendcolor[2] = b.b * FRACUNIT / 255;
				info.blend = BLEND_MODULATE;
			}
			else
			{
				fixed_t blendalpha = b.a * FRACUNIT / 255;
				info.blendcolor[0] = b.r * blendalpha;
				info.blendcolor[1] = b.g * blendalpha;
				info.blendcolor[2] = b.b * blendalpha;
				info.blendcolor[3] = FRACUNIT - blendalpha;
				info.blend = BLEND_OVERLAY;
			}
		}

		if (Parts[i].Translation != NULL)
		{ // Using a translation forces downconversion to the base palette
			ret = Parts[i].Texture->CopyTrueColorTranslated(bmp, x+Parts[i].OriginX, y+Parts[i].OriginY, Parts[i].Rotate, Parts[i].Translation, &info);
		}
		else
		{
			ret = Parts[i].Texture->CopyTrueColorPixels(bmp, x+Parts[i].OriginX, y+Parts[i].OriginY, Parts[i].Rotate, &info);
		}

		if (ret > retv) retv = ret;
	}
	// Restore previous clipping rectangle.
	bmp->SetClipRect(saved_cr);
	return retv;
}

//==========================================================================
//
// FMultiPatchTexture :: GetFormat
//
// only returns 'paletted' if all patches use the base palette.
//
//==========================================================================

FTextureFormat FMultiPatchTexture::GetFormat() 
{ 
	if (bComplex) return TEX_RGB;
	if (NumParts == 1) return Parts[0].Texture->GetFormat();
	return UseBasePalette() ? TEX_Pal : TEX_RGB;
}


//===========================================================================
//
// FMultipatchTexture::UseBasePalette
//
// returns true if all patches in the texture use the unmodified base
// palette.
//
//===========================================================================

bool FMultiPatchTexture::UseBasePalette() 
{ 
	if (bComplex) return false;
	for(int i=0;i<NumParts;i++)
	{
		if (!Parts[i].Texture->UseBasePalette()) return false;
	}
	return true;
}

//==========================================================================
//
// FMultiPatchTexture :: CheckForHacks
//
//==========================================================================

void FMultiPatchTexture::CheckForHacks ()
{
	if (NumParts <= 0)
	{
		return;
	}

	// Heretic sky textures are marked as only 128 pixels tall,
	// even though they are really 200 pixels tall.
	if (gameinfo.gametype == GAME_Heretic &&
		Name[0] == 'S' &&
		Name[1] == 'K' &&
		Name[2] == 'Y' &&
		Name[4] == 0 &&
		Name[3] >= '1' &&
		Name[3] <= '3' &&
		Height == 128)
	{
		Height = 200;
		HeightBits = 8;
		return;
	}

	// The Doom E1 sky has its patch's y offset at -8 instead of 0.
	if (gameinfo.gametype == GAME_Doom &&
		!(gameinfo.flags & GI_MAPxx) &&
		NumParts == 1 &&
		Height == 128 &&
		Parts->OriginY == -8 &&
		Name[0] == 'S' &&
		Name[1] == 'K' &&
		Name[2] == 'Y' &&
		Name[3] == '1' &&
		Name[4] == 0)
	{
		Parts->OriginY = 0;
		return;
	}

	// BIGDOOR7 in Doom also has patches at y offset -4 instead of 0.
	if (gameinfo.gametype == GAME_Doom &&
		!(gameinfo.flags & GI_MAPxx) &&
		NumParts == 2 &&
		Height == 128 &&
		Parts[0].OriginY == -4 &&
		Parts[1].OriginY == -4 &&
		Name[0] == 'B' &&
		Name[1] == 'I' &&
		Name[2] == 'G' &&
		Name[3] == 'D' &&
		Name[4] == 'O' &&
		Name[5] == 'O' &&
		Name[6] == 'R' &&
		Name[7] == '7')
	{
		Parts[0].OriginY = 0;
		Parts[1].OriginY = 0;
		return;
	}

	// [RH] Some wads (I forget which!) have single-patch textures 256
	// pixels tall that have patch lengths recorded as 0. I can't think of
	// any good reason for them to do this, and since I didn't make note
	// of which wad made me hack in support for them, the hack is gone
	// because I've added support for DeePsea's true tall patches.
	//
	// Okay, I found a wad with crap patches: Pleiades.wad's sky patches almost
	// fit this description and are a big mess, but they're not single patch!
	if (Height == 256)
	{
		int i;

		// All patches must be at the top of the texture for this fix
		for (i = 0; i < NumParts; ++i)
		{
			if (Parts[i].OriginY != 0)
			{
				break;
			}
		}

		if (i == NumParts)
		{
			for (i = 0; i < NumParts; ++i)
			{
				Parts[i].Texture->HackHack(256);
			}
		}
	}
}

//==========================================================================
//
// FMultiPatchTexture :: GetRedirect
//
//==========================================================================

FTexture *FMultiPatchTexture::GetRedirect(bool wantwarped)
{
	return bRedirect ? Parts->Texture : this;
}

//==========================================================================
//
// FMultiPatchTexture :: GetRawTexture
//
// Doom ignored all compositing of mid-sided textures on two-sided lines.
// Since these textures had to be single-patch in Doom, that essentially
// means it ignores their Y offsets.
//
// If this texture is composed of only one patch, return that patch.
// Otherwise, return this texture, since Doom wouldn't have been able to
// draw it anyway.
//
//==========================================================================

FTexture *FMultiPatchTexture::GetRawTexture()
{
	return NumParts == 1 ? Parts->Texture : this;
}

//==========================================================================
//
// FMultiPatchTexture :: TexPart :: TexPart
//
//==========================================================================

FMultiPatchTexture::TexPart::TexPart()
{
	OriginX = OriginY = 0;
	Rotate = 0;
	Texture = NULL;
	Translation = NULL;
	Blend = 0;
	Alpha = FRACUNIT;
	op = OP_COPY;
}

//==========================================================================
//
// FTextureManager :: AddTexturesLump
//
//==========================================================================

void FTextureManager::AddTexturesLump (const void *lumpdata, int lumpsize, int deflumpnum, int patcheslump, int firstdup, bool texture1)
{
	FPatchLookup *patchlookup = NULL;
	int i;
	DWORD numpatches;

	if (firstdup == 0)
	{
		firstdup = (int)Textures.Size();
	}

	{
		FWadLump pnames = Wads.OpenLumpNum (patcheslump);

		pnames >> numpatches;

		// Check whether the amount of names reported is correct.
		if ((signed)numpatches < 0)
		{
			Printf("Corrupt PNAMES lump found (negative amount of entries reported)");
			return;
		}

		// Check whether the amount of names reported is correct.
		int lumplength = Wads.LumpLength(patcheslump);
		if (numpatches > DWORD((lumplength-4)/8))
		{
			Printf("PNAMES lump is shorter than required (%u entries reported but only %d bytes (%d entries) long\n",
				numpatches, lumplength, (lumplength-4)/8);
			// Truncate but continue reading. Who knows how many such lumps exist?
			numpatches = (lumplength-4)/8;
		}

		// Catalog the patches these textures use so we know which
		// textures they represent.
		patchlookup = new FPatchLookup[numpatches];
		for (DWORD i = 0; i < numpatches; ++i)
		{
			char pname[9];
			pnames.Read(pname, 8);
			pname[8] = '\0';
			patchlookup[i].Name = pname;
			FTextureID j = CheckForTexture (patchlookup[i].Name, FTexture::TEX_WallPatch);
			if (j.isValid())
			{
				patchlookup[i].Texture = Textures[j.GetIndex()].Texture;
			}
			else
			{
				// Shareware Doom has the same PNAMES lump as the registered
				// Doom, so printing warnings for patches that don't really
				// exist isn't such a good idea.
				//Printf ("Patch %s not found.\n", patchlookup[i].Name);
				patchlookup[i].Texture = NULL;
			}
		}
	}

	bool isStrife = false;
	const DWORD *maptex, *directory;
	DWORD maxoff;
	int numtextures;
	DWORD offset = 0;   // Shut up, GCC!

	maptex = (const DWORD *)lumpdata;
	numtextures = LittleLong(*maptex);
	maxoff = lumpsize;

	if (maxoff < DWORD(numtextures+1)*4)
	{
		Printf ("Texture directory is too short");
		delete[] patchlookup;
		return;
	}

	// Scan the texture lump to decide if it contains Doom or Strife textures
	for (i = 0, directory = maptex+1; i < numtextures; ++i)
	{
		offset = LittleLong(directory[i]);
		if (offset > maxoff)
		{
			Printf ("Bad texture directory");
			delete[] patchlookup;
			return;
		}

		maptexture_t *tex = (maptexture_t *)((BYTE *)maptex + offset);

		// There is bizzarely a Doom editing tool that writes to the
		// first two elements of columndirectory, so I can't check those.
		if (SAFESHORT(tex->patchcount) < 0 ||
			tex->columndirectory[2] != 0 ||
			tex->columndirectory[3] != 0)
		{
			isStrife = true;
			break;
		}
	}


	// Textures defined earlier in the lump take precedence over those defined later,
	// but later TEXTUREx lumps take precedence over earlier ones.
	for (i = 1, directory = maptex; i <= numtextures; ++i)
	{
		if (i == 1 && texture1)
		{
			// The very first texture is just a dummy. Copy its dimensions to texture 0.
			// It still needs to be created in case someone uses it by name.
			offset = LittleLong(directory[1]);
			const maptexture_t *tex = (const maptexture_t *)((const BYTE *)maptex + offset);
			FDummyTexture *tex0 = static_cast<FDummyTexture *>(Textures[0].Texture);
			tex0->SetSize (SAFESHORT(tex->width), SAFESHORT(tex->height));
		}

		offset = LittleLong(directory[i]);
		if (offset > maxoff)
		{
			Printf ("Bad texture directory");
			delete[] patchlookup;
			return;
		}

		// If this texture was defined already in this lump, skip it
		// This could cause problems with animations that use the same name for intermediate
		// textures. Should I be worried?
		int j;
		for (j = (int)Textures.Size() - 1; j >= firstdup; --j)
		{
			if (strnicmp (Textures[j].Texture->Name, (const char *)maptex + offset, 8) == 0)
				break;
		}
		if (j + 1 == firstdup)
		{
			FMultiPatchTexture *tex = new FMultiPatchTexture ((const BYTE *)maptex + offset, patchlookup, numpatches, isStrife, deflumpnum);
			if (i == 1 && texture1)
			{
				tex->UseType = FTexture::TEX_FirstDefined;
			}
			TexMan.AddTexture (tex);
			StartScreen->Progress();
		}
	}
	delete[] patchlookup;
}


//==========================================================================
//
// FTextureManager :: AddTexturesLumps
//
//==========================================================================

void FTextureManager::AddTexturesLumps (int lump1, int lump2, int patcheslump)
{
	int firstdup = (int)Textures.Size();

	if (lump1 >= 0)
	{
		FMemLump texdir = Wads.ReadLump (lump1);
		AddTexturesLump (texdir.GetMem(), Wads.LumpLength (lump1), lump1, patcheslump, firstdup, true);
	}
	if (lump2 >= 0)
	{
		FMemLump texdir = Wads.ReadLump (lump2);
		AddTexturesLump (texdir.GetMem(), Wads.LumpLength (lump2), lump2, patcheslump, firstdup, false);
	}
}


//==========================================================================
//
// 
//
//==========================================================================

void FMultiPatchTexture::ParsePatch(FScanner &sc, TexPart & part, bool silent, int usetype)
{
	FString patchname;
	sc.MustGetString();

	FTextureID texno = TexMan.CheckForTexture(sc.String, usetype);
	int Mirror = 0;

	if (!texno.isValid())
	{
		if (strlen(sc.String) <= 8 && !strpbrk(sc.String, "./"))
		{
			int lumpnum = Wads.CheckNumForName(sc.String, usetype == TEX_MiscPatch? ns_graphics : ns_patches);
			if (lumpnum >= 0)
			{
				part.Texture = FTexture::CreateTexture(lumpnum, usetype);
				TexMan.AddTexture(part.Texture);
			}
		}
	}
	else
	{
		part.Texture = TexMan[texno];
		bComplex |= part.Texture->bComplex;
	}
	if (part.Texture == NULL)
	{
		if (!silent) Printf(TEXTCOLOR_RED "Unknown patch '%s' in texture '%s'\n", sc.String, Name.GetChars());
	}
	sc.MustGetStringName(",");
	sc.MustGetNumber();
	part.OriginX = sc.Number;
	sc.MustGetStringName(",");
	sc.MustGetNumber();
	part.OriginY = sc.Number;

	if (sc.CheckString("{"))
	{
		while (!sc.CheckString("}"))
		{
			sc.MustGetString();
			if (sc.Compare("flipx"))
			{
				Mirror |= 1;
			}
			else if (sc.Compare("flipy"))
			{
				Mirror |= 2;
			}
			else if (sc.Compare("rotate"))
			{
				sc.MustGetNumber();
				sc.Number = (((sc.Number + 90)%360)-90);
 				if (sc.Number != 0 && sc.Number !=90 && sc.Number != 180 && sc.Number != -90)
 				{
					sc.ScriptError("Rotation must be a multiple of 90 degrees.");
				}
				part.Rotate = (sc.Number / 90) & 3;
			}
			else if (sc.Compare("Translation"))
			{
				int match;

				bComplex = true;
				if (part.Translation != NULL) delete part.Translation;
				part.Translation = NULL;
				part.Blend = 0;
				static const char *maps[] = { "inverse", "gold", "red", "green", "blue", NULL };
				sc.MustGetString();

				match = sc.MatchString(maps);
				if (match >= 0)
				{
					part.Blend = BLEND_SPECIALCOLORMAP1 + match;
				}
				else if (sc.Compare("ICE"))
				{
					part.Blend = BLEND_ICEMAP;
				}
				else if (sc.Compare("DESATURATE"))
				{
					sc.MustGetStringName(",");
					sc.MustGetNumber();
					part.Blend = BLEND_DESATURATE1 + clamp(sc.Number-1, 0, 30);
				}
				else
				{
					sc.UnGet();
					part.Translation = new FRemapTable;
					part.Translation->MakeIdentity();
					do
					{
						sc.MustGetString();
						part.Translation->AddToTranslation(sc.String);
					}
					while (sc.CheckString(","));
				}

			}
			else if (sc.Compare("Colormap"))
			{
				float r1,g1,b1;
				float r2,g2,b2;

				sc.MustGetFloat();
				r1 = (float)sc.Float;
				sc.MustGetStringName(",");
				sc.MustGetFloat();
				g1 = (float)sc.Float;
				sc.MustGetStringName(",");
				sc.MustGetFloat();
				b1 = (float)sc.Float;
				if (!sc.CheckString(","))
				{
					part.Blend = AddSpecialColormap(0,0,0, r1, g1, b1);
				}
				else
				{
					sc.MustGetFloat();
					r2 = (float)sc.Float;
					sc.MustGetStringName(",");
					sc.MustGetFloat();
					g2 = (float)sc.Float;
					sc.MustGetStringName(",");
					sc.MustGetFloat();
					b2 = (float)sc.Float;
					part.Blend = AddSpecialColormap(r1, g1, b1, r2, g2, b2);
				}
			}
			else if (sc.Compare("Blend"))
			{
				bComplex = true;
				if (part.Translation != NULL) delete part.Translation;
				part.Translation = NULL;
				part.Blend = 0;

				if (!sc.CheckNumber())
				{
					sc.MustGetString();
					part.Blend = V_GetColor(NULL, sc.String);
				}
				else
				{
					int r,g,b;

					r = sc.Number;
					sc.MustGetStringName(",");
					sc.MustGetNumber();
					g = sc.Number;
					sc.MustGetStringName(",");
					sc.MustGetNumber();
					b = sc.Number;
					sc.MustGetStringName(",");
					part.Blend = MAKERGB(r, g, b);
				}
				// Blend.a may never be 0 here.
				if (sc.CheckString(","))
				{
					sc.MustGetFloat();
					if (sc.Float > 0.f)
						part.Blend.a = clamp<int>(int(sc.Float*255), 1, 254);
					else
						part.Blend = 0;
				}
				else part.Blend.a = 255;
			}
			else if (sc.Compare("alpha"))
			{
				sc.MustGetFloat();
				part.Alpha = clamp(FLOAT2FIXED(sc.Float), 0, FRACUNIT);
				// bComplex is not set because it is only needed when the style is not OP_COPY.
			}
			else if (sc.Compare("style"))
			{
				static const char *styles[] = {"copy", "translucent", "add", "subtract", "reversesubtract", "modulate", "copyalpha", "copynewalpha", "overlay", NULL };
				sc.MustGetString();
				part.op = sc.MustMatchString(styles);
				bComplex |= (part.op != OP_COPY);
				bTranslucentPatches = bComplex;
			}
			else if (sc.Compare("useoffsets"))
			{
				if (part.Texture != NULL)
				{
					part.OriginX -= part.Texture->LeftOffset;
					part.OriginY -= part.Texture->TopOffset;
				}
			}
		}
	}
	if (Mirror & 2)
	{
		part.Rotate = (part.Rotate + 2) & 3;
		Mirror ^= 1;
	}
	if (Mirror & 1)
	{
		part.Rotate |= 4;
	}
}

//==========================================================================
//
// Constructor for text based multipatch definitions
//
//==========================================================================

FMultiPatchTexture::FMultiPatchTexture (FScanner &sc, int usetype)
: Pixels (0), Spans(0), Parts(0), bRedirect(false), bTranslucentPatches(false)
{
	TArray<TexPart> parts;
	bool bSilent = false;

	bMultiPatch = true;
	sc.SetCMode(true);
	sc.MustGetString();
	const char* textureName = NULL;
	if (sc.Compare("optional"))
	{
		bSilent = true;
		sc.MustGetString();
		if (sc.Compare(","))
		{
			// this is not right. Apparently a texture named 'optional' is being defined right now...
			sc.UnGet();
			textureName = "optional";
			bSilent = false;
		}
	}
	Name = !textureName ? sc.String : textureName;
	Name.ToUpper();
	sc.MustGetStringName(",");
	sc.MustGetNumber();
	Width = sc.Number;
	sc.MustGetStringName(",");
	sc.MustGetNumber();
	Height = sc.Number;
	UseType = usetype;
	
	if (sc.CheckString("{"))
	{
		while (!sc.CheckString("}"))
		{
			sc.MustGetString();
			if (sc.Compare("XScale"))
			{
				sc.MustGetFloat();
				xScale = FLOAT2FIXED(sc.Float);
				if (xScale == 0) sc.ScriptError("Texture %s is defined with null x-scale\n", Name.GetChars());
			}
			else if (sc.Compare("YScale"))
			{
				sc.MustGetFloat();
				yScale = FLOAT2FIXED(sc.Float);
				if (yScale == 0) sc.ScriptError("Texture %s is defined with null y-scale\n", Name.GetChars());
			}
			else if (sc.Compare("WorldPanning"))
			{
				bWorldPanning = true;
			}
			else if (sc.Compare("NullTexture"))
			{
				UseType = FTexture::TEX_Null;
			}
			else if (sc.Compare("NoDecals"))
			{
				bNoDecals = true;
			}
			else if (sc.Compare("Patch"))
			{
				TexPart part;
				ParsePatch(sc, part, bSilent, TEX_WallPatch);
				if (part.Texture != NULL) parts.Push(part);
				part.Texture = NULL;
				part.Translation = NULL;
			}
			else if (sc.Compare("Graphic"))
			{
				TexPart part;
				ParsePatch(sc, part, bSilent, TEX_MiscPatch);
				if (part.Texture != NULL) parts.Push(part);
				part.Texture = NULL;
				part.Translation = NULL;
			}
			else if (sc.Compare("Offset"))
			{
				sc.MustGetNumber();
				LeftOffset = sc.Number;
				sc.MustGetStringName(",");
				sc.MustGetNumber();
				TopOffset = sc.Number;
			}
			else
			{
				sc.ScriptError("Unknown texture property '%s'", sc.String);
			}
		}

		NumParts = parts.Size();
		Parts = new TexPart[NumParts];
		memcpy(Parts, &parts[0], NumParts * sizeof(*Parts));

		//CalcBitSize ();

		// If this texture is just a wrapper around a single patch, we can simply
		// forward GetPixels() and GetColumn() calls to that patch.
		if (NumParts == 1)
		{
			if (Parts->OriginX == 0 && Parts->OriginY == 0 &&
				Parts->Texture->GetWidth() == Width &&
				Parts->Texture->GetHeight() == Height &&
				Parts->Rotate == 0 && 
				!bComplex)
			{
				bRedirect = true;
			}
		}
	}
	
	if (Width <= 0 || Height <= 0)
	{
		UseType = FTexture::TEX_Null;
		Printf("Texture %s has invalid dimensions (%d, %d)\n", Name.GetChars(), Width, Height);
		Width = Height = 1;
	}
	CalcBitSize ();

	
	sc.SetCMode(false);
}



void FTextureManager::ParseXTexture(FScanner &sc, int usetype)
{
	FTexture *tex = new FMultiPatchTexture(sc, usetype);
	TexMan.AddTexture (tex);
}
