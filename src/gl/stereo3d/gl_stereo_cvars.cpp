#include "gl/stereo3d/gl_stereo3d.h"
#include "gl/stereo3d/gl_stereo_leftright.h"
#include "gl/system/gl_cvars.h"

CVAR(Int, vr_mode, 0, CVAR_GLOBALCONFIG)
CVAR(Bool, vr_swap, false, CVAR_GLOBALCONFIG)
// intraocular distance in meters
CVAR(Float, vr_ipd, 0.062f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // METERS

namespace s3d {

// Initialize static member
Stereo3DMode const * Stereo3DMode::currentStereo3DMode = nullptr;

/* static */
void Stereo3DMode::setCurrentMode(const Stereo3DMode& mode) {
	Stereo3DMode::currentStereo3DMode = &mode;
}

/* static */
const Stereo3DMode& Stereo3DMode::getCurrentMode() {

	switch (vr_mode)
	{
	case 1:
		setCurrentMode(LeftEyeView::getInstance(vr_ipd));
		break;
	default:
	case 0:
		setCurrentMode(MonoView::getInstance());
		break;
	}
	return *currentStereo3DMode;
}

} /* namespace s3d */

