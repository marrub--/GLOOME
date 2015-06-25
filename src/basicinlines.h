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

#ifndef ZDOOM_SRC_BASIC_INLINES_H__
#define ZDOOM_SRC_BASIC_INLINES_H__

#if defined(__GNUC__) && !defined(__forceinline)
#define __forceinline __inline__ __attribute__((always_inline))
#endif

static __forceinline SDWORD Scale(SDWORD x, SDWORD y, SDWORD n)
{
	return (SDWORD)(((SQWORD)x*y) / n);
}

static __forceinline SDWORD MulScale(SDWORD x, SDWORD y, SDWORD n)
{
	return (SDWORD)(((SQWORD)x*y) >> n);
}

#define MulScaleN(n) \
static __forceinline SDWORD MulScale##n	(SDWORD x, SDWORD y) \
{ \
	return (SDWORD)(((SQWORD)x*y) >> n); \
}

MulScaleN(1)
MulScaleN(2)
MulScaleN(3)
MulScaleN(4)
MulScaleN(5)
MulScaleN(6)
MulScaleN(7)
MulScaleN(8)
MulScaleN(9)
MulScaleN(10)
MulScaleN(11)
MulScaleN(12)
MulScaleN(13)
MulScaleN(14)
MulScaleN(15)
MulScaleN(16)
MulScaleN(17)
MulScaleN(18)
MulScaleN(19)
MulScaleN(20)
MulScaleN(21)
MulScaleN(22)
MulScaleN(23)
MulScaleN(24)
MulScaleN(25)
MulScaleN(26)
MulScaleN(27)
MulScaleN(28)
MulScaleN(29)
MulScaleN(30)
MulScaleN(31)
MulScaleN(32)

#undef MulScaleN

static __forceinline DWORD UMulScale16(DWORD a, DWORD b)
{
	return (DWORD)(((QWORD)a * b) >> 16);
}

static __forceinline SDWORD DMulScale(SDWORD x, SDWORD y, SDWORD a, SDWORD b, SDWORD n)
{
	return (SDWORD)(((SQWORD)x*y + (SQWORD)a*b) >> n);
}

#define DMulScaleN(n) \
static __forceinline SDWORD DMulScale##n	(SDWORD x, SDWORD y, SDWORD a, SDWORD b) \
{ \
	return (SDWORD)(((SQWORD)x*y + (SQWORD)a*b) >> n); \
}

DMulScaleN(1)
DMulScaleN(2)
DMulScaleN(3)
DMulScaleN(4)
DMulScaleN(5)
DMulScaleN(6)
DMulScaleN(7)
DMulScaleN(8)
DMulScaleN(9)
DMulScaleN(10)
DMulScaleN(11)
DMulScaleN(12)
DMulScaleN(13)
DMulScaleN(14)
DMulScaleN(15)
DMulScaleN(16)
DMulScaleN(17)
DMulScaleN(18)
DMulScaleN(19)
DMulScaleN(20)
DMulScaleN(21)
DMulScaleN(22)
DMulScaleN(23)
DMulScaleN(24)
DMulScaleN(25)
DMulScaleN(26)
DMulScaleN(27)
DMulScaleN(28)
DMulScaleN(29)
DMulScaleN(30)
DMulScaleN(31)
DMulScaleN(32)

#undef DMulScaleN

#define TMulScaleN(n) \
static __forceinline SDWORD TMulScale##n (SDWORD x, SDWORD y, SDWORD a, SDWORD b, SDWORD q, SDWORD r) \
{ \
	return (SDWORD)(((SQWORD)x*y + (SQWORD)a*b + (SQWORD)q*r) >> n); \
}

TMulScaleN(1)
TMulScaleN(2)
TMulScaleN(3)
TMulScaleN(4)
TMulScaleN(5)
TMulScaleN(6)
TMulScaleN(7)
TMulScaleN(8)
TMulScaleN(9)
TMulScaleN(10)
TMulScaleN(11)
TMulScaleN(12)
TMulScaleN(13)
TMulScaleN(14)
TMulScaleN(15)
TMulScaleN(16)
TMulScaleN(17)
TMulScaleN(18)
TMulScaleN(19)
TMulScaleN(20)
TMulScaleN(21)
TMulScaleN(22)
TMulScaleN(23)
TMulScaleN(24)
TMulScaleN(25)
TMulScaleN(26)
TMulScaleN(27)
TMulScaleN(28)
TMulScaleN(29)
TMulScaleN(30)
TMulScaleN(31)
TMulScaleN(32)

#undef TMulScaleN

static __forceinline SDWORD BoundMulScale(SDWORD x, SDWORD y, SDWORD n)
{
	SQWORD q = ((SQWORD)x * y) >> n;
	return (q > (0x7FFFFFFF)) ? (0x7FFFFFFF) : ((q < (-0x80000000LL)) ? (0x80000000) : (SDWORD)(q));
}

static inline SDWORD DivScale(SDWORD x, SDWORD y, SDWORD n)
{
	return (SDWORD)(((SQWORD)x << n) / y);
}

#define DivScaleN(n) \
static inline SDWORD DivScale##n	(SDWORD x, SDWORD y) \
{ \
	return (SDWORD)(((SQWORD)x << n) / y); \
}

DivScaleN(1)
DivScaleN(2)
DivScaleN(3)
DivScaleN(4)
DivScaleN(5)
DivScaleN(6)
DivScaleN(7)
DivScaleN(8)
DivScaleN(9)
DivScaleN(10)
DivScaleN(11)
DivScaleN(12)
DivScaleN(13)
DivScaleN(14)
DivScaleN(15)
DivScaleN(16)
DivScaleN(17)
DivScaleN(18)
DivScaleN(19)
DivScaleN(20)
DivScaleN(21)
DivScaleN(22)
DivScaleN(23)
DivScaleN(24)
DivScaleN(25)
DivScaleN(26)
DivScaleN(27)
DivScaleN(28)
DivScaleN(29)
DivScaleN(30)
DivScaleN(31)
DivScaleN(32)

#undef DivScaleN

static __forceinline void clearbuf(void *bf, unsigned int count, SDWORD clear)
{
	SDWORD *bufp = (SDWORD *)bf;
	for (unsigned int i = 0; i != count; ++i)
	{
		bufp[i] = clear;
	}
}

static __forceinline void clearbufshort(void *bf, unsigned int count, WORD clear)
{
	SWORD *bufp = (SWORD *)bf;
	for (unsigned int i = 0; i != count; ++i)
	{
		bufp[i] = clear;
	}
}

static __forceinline SDWORD ksgn(SDWORD x)
{
	if(x < 0)
	{
		return -1;
	}
	else if(x > 0)
	{
		return 1;
	}
	
	return 0;
}

#endif
