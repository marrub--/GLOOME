#ifndef GL_ANAGLYPH_H_
#define GL_ANAGLYPH_H_

#include "gl_stereo3d.h"


namespace s3d {


class LeftEye : public Stereo3DMode
{
public:
	LeftEye(double ipdMeters);
};


class RightEye : public Stereo3DMode
{
public:
	RightEye(double ipdMeters);
};


class ColorMask
{
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
};


// TODO matrix anaglyph


} /* namespace st3d */


#endif /* GL_ANAGLYPH_H_ */
