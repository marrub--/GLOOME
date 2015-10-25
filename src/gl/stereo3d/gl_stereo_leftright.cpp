#include "gl_stereo_leftright.h"
#include "vectors.h" // RAD2DEG
#include "doomtype.h" // M_PI
#include "gl/system/gl_cvars.h"

EXTERN_CVAR(Float, vr_screendist)

namespace s3d {


// TODO: actually implement asymmetric frustum shift
/* virtual */
void ShiftedEyePose::GetProjection(float fov, float aspectRatio, float fovRatio, GLdouble m[4][4]) const
{
	// Lifted from gl_scene.cpp FGLRenderer::SetProjection()
	float fovy = 2 * RAD2DEG(atan(tan(DEG2RAD(fov) / 2) / fovRatio));
	const double zNear = 5.0;
	const double zFar = 65536.0;

	// For stereo 3D, use asymmetric frustum shift in projection matrix
	// Q: shouldn't shift vary with roll angle, at least for desktop display?
	// A: (lab) roll is not measured on desktop display (yet)
	const double frustumShift = zNear * shift / vr_screendist; // meters cancel
	double fH = tan(fovy / 360 * M_PI) * zNear;
	double fW = fH * aspectRatio;
	// glFrustum(-fW - frustumShift, fW - frustumShift, -fH, fH, zNear, zFar);
	double left = -fW - frustumShift;
	double right = fW - frustumShift;
	double bottom = -fH;
	double top = fH;
	double deltaZ = zFar - zNear;

	memset(m, 0, 16 * sizeof(GLdouble));

	m[0][0] = 2 * zNear / (right - left);
	m[1][1] = 2 * zNear / (top - bottom);
	m[2][2] = -(zFar + zNear) / deltaZ;
	m[2][3] = -1;
	m[3][2] = -2 * zNear * zFar / deltaZ;
	// m[3][3] = 0; // redundant
	// m[2][1] = (top + bottom) / (top - bottom); // zero for the cases I know of...
	m[2][0] = (right + left) / (right - left); // asymmetric shift is here
}


/* static */
const LeftEyeView& LeftEyeView::getInstance(float ipd)
{
	static LeftEyeView instance(ipd);
	instance.setIpd(ipd);
	return instance;
}

} /* namespace s3d */
