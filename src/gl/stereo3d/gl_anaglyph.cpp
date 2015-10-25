#include "gl_anaglyph.h"

namespace s3d {

	MaskAnaglyph::MaskAnaglyph(const ColorMask& leftColorMask, double ipdMeters)
		: leftEye(ipdMeters), rightEye(ipdMeters)
	{
		eye_ptrs.push_back(&leftEye);
		eye_ptrs.push_back(&rightEye);
	}

} /* namespace s3d */
