#ifndef GL_ANAGLYPH_H_
#define GL_ANAGLYPH_H_

#include "gl_stereo3d.h"
#include "gl_stereo_leftright.h"


namespace s3d {


class ColorMask
{
public:
	ColorMask(bool r, bool g, bool b) : redMask(r), greenMask(g), blueMask(b) {}
private:
	bool redMask;
	bool greenMask;
	bool blueMask;
};


class MaskAnaglyph : public Stereo3DMode
{
public:
	MaskAnaglyph(const ColorMask& leftColorMask, double ipdMeters);
private:
	LeftEyePose leftEye;
	RightEyePose rightEye;
};


class RedCyan : public MaskAnaglyph
{
public:
	static const RedCyan& getInstance(float ipd);

	RedCyan(float ipd) : MaskAnaglyph(ColorMask(true, false, false), ipd) {}
};

class GreenMagenta : public MaskAnaglyph
{
public:
	static const GreenMagenta& getInstance(float ipd);

	GreenMagenta(float ipd) : MaskAnaglyph(ColorMask(false, true, false), ipd) {}
};

// TODO matrix anaglyph


} /* namespace st3d */


#endif /* GL_ANAGLYPH_H_ */
