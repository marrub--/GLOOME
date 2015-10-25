#include "gl/stereo3d/gl_stereo3d.h"
#include "vectors.h" // RAD2DEG
#include "doomtype.h" // M_PI

namespace s3d {


/* virtual */
void EyePose::GetProjection(float fov, float aspectRatio, float fovRatio, GLdouble m[4][4]) const
{
	// Lifted from gl_scene.cpp FGLRenderer::SetProjection()
	float fovy = 2 * RAD2DEG(atan(tan(DEG2RAD(fov) / 2) / fovRatio));
	const double zNear = 5.0;
	const double zFar = 65536.0;

	double sine, cotangent, deltaZ;
	double radians = fovy / 2 * M_PI / 180;

	deltaZ = zFar - zNear;
	sine = sin(radians);
	if ((deltaZ == 0) || (sine == 0) || (aspectRatio == 0)) {
		return;
	}
	cotangent = cos(radians) / sine;

	memset(m, 0, 16*sizeof(GLdouble));
	m[0][0] = cotangent / aspectRatio;
	m[1][1] = cotangent;
	m[2][2] = -(zFar + zNear) / deltaZ;
	m[2][3] = -1;
	m[3][2] = -2 * zNear * zFar / deltaZ;
	m[3][3] = 0;
}

/* virtual */
Viewport EyePose::GetViewport(const Viewport& fullViewport) const 
{
	return fullViewport;
}


/* virtual */
void EyePose::GetView(
	float viewx, float viewy, float viewz,
	float roll, float pitch, float yaw,
	float outViewPos[3], float outRollPitchYaw[3]) const
{
	// pass-through for Mono view
	outViewPos[0] = viewx;
	outViewPos[1] = viewy;
	outViewPos[2] = viewz;
	outRollPitchYaw[0] = roll;
	outRollPitchYaw[1] = pitch;
	outRollPitchYaw[2] = yaw;
}


Stereo3DMode::Stereo3DMode()
{
}

Stereo3DMode::~Stereo3DMode()
{
}

// Avoid static initialization order fiasco by declaring first Mode type (Mono) here in the
// same source file as Stereo3DMode::getCurrentMode()
// https://isocpp.org/wiki/faq/ctors#static-init-order
MonoView::MonoView() 
{
}

/* static */
const MonoView& MonoView::getInstance() 
{
	static MonoView instance;
	return instance;
}

} /* namespace s3d */
