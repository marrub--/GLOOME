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

#ifndef ZDOOM_SRC_M_FIXED_H__
#define ZDOOM_SRC_M_FIXED_H__

#include <stdlib.h>
#include "doomtype.h"
#include "basicinlines.h"
#include "xs_Float.h"

#define FixedMul MulScale16
#define FixedDiv SafeDivScale16
#define FLOAT2FIXED(f) xs_Fix<16>::ToFix(f)
#define FIXED2FLOAT(f) ((f) / (float)(65536))
#define FIXED2DBL(f)   ((f) / (double)(65536))

#define SafeDivScaleN(n) \
static inline SDWORD SafeDivScale##n	(SDWORD x, SDWORD y) \
{ \
	if((DWORD)abs(x) >> (31 - n) >= (DWORD)abs(y)) \
	{ \
		return ((x ^ y) < 0) ? (FIXED_MIN) : (FIXED_MAX); \
	} \
	\
	return DivScale##n	(x, y); \
}

SafeDivScaleN(1)
SafeDivScaleN(2)
SafeDivScaleN(3)
SafeDivScaleN(4)
SafeDivScaleN(5)
SafeDivScaleN(6)
SafeDivScaleN(7)
SafeDivScaleN(8)
SafeDivScaleN(9)
SafeDivScaleN(10)
SafeDivScaleN(11)
SafeDivScaleN(12)
SafeDivScaleN(13)
SafeDivScaleN(14)
SafeDivScaleN(15)
SafeDivScaleN(16)
SafeDivScaleN(17)
SafeDivScaleN(18)
SafeDivScaleN(19)
SafeDivScaleN(20)
SafeDivScaleN(21)
SafeDivScaleN(22)
SafeDivScaleN(23)
SafeDivScaleN(24)
SafeDivScaleN(25)
SafeDivScaleN(26)
SafeDivScaleN(27)
SafeDivScaleN(28)
SafeDivScaleN(29)
SafeDivScaleN(30)

#undef SafeDivScaleN

static inline SDWORD SafeDivScale31(SDWORD x, SDWORD y)
{
	if((DWORD)abs(x) >= (DWORD)abs(y))
	{
		return ((x ^ y) < 0) ? (FIXED_MIN) : (FIXED_MAX);
	}
	
	return DivScale31(x, y);
}

static inline SDWORD SafeDivScale32(SDWORD x, SDWORD y)
{
	if((DWORD)abs(x) >= (DWORD)abs(y) >> 1)
	{
		return ((x^y) < 0) ? (FIXED_MIN) : (FIXED_MAX);
	}
	
	return DivScale32(x, y);
}

static inline void qinterpolatedown16(SDWORD *out, DWORD num, SDWORD v, SDWORD d)
{
	DWORD n = num;
	
	if(!n)
	{
		return;
	}
	
	if(n & 1)
	{
		*out = (v >> 16);
		v += d;
	}
	
	for(n = n >> 1; n-- != 0;)
	{
		int temp = v + d;
		
		*out = v >> 16;
		v = temp + d;
		*(out + 1) = temp >> 16;
		
		out += 2;
	}
}

static inline void qinterpolatedown16short(short *out, DWORD num, SDWORD v, SDWORD d)
{
	DWORD n = num;
	
	if(!n)
	{
		return;
	}
	
	if((DWORD)out & 2)
	{
		*(out++) = (short)(v >> 16);
		n--, v += d;
	}
	
	DWORD *out2 = (DWORD *)out;
	DWORD n2;
	for(n2 = (n >> 1); n2-- != 0;)
	{
		int temp = v + d;
		
		*(out2++) = (temp & 0xFFFF0000) | ((DWORD)v >> 16);
		v = temp + d;
	}
	
	if(n & 1)
	{
		short *out3 = (short *)out2;
		*out3 = (short)(v >> 16);
	}
}

#endif
