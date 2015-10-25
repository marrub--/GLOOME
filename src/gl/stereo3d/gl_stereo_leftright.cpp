#include "gl_stereo_leftright.h"
#include "vectors.h" // RAD2DEG
#include "doomtype.h" // M_PI


namespace s3d {


// TODO: actually implement asymmetric frustum shift
/* virtual */
void ShiftedEyePose::GetProjection(float fov, float aspectRatio, float fovRatio, GLdouble m[4][4]) const
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

	memset(m, 0, 16 * sizeof(GLdouble));
	m[0][0] = cotangent / aspectRatio;
	m[1][1] = cotangent;
	m[2][2] = -(zFar + zNear) / deltaZ;
	m[2][3] = -1;
	m[3][2] = -2 * zNear * zFar / deltaZ;
	m[3][3] = 0;
}


/* static */
const LeftEyeView& LeftEyeView::getInstance(float ipd)
{
	static LeftEyeView instance(ipd);
	instance.setIpd(ipd);
	return instance;
}

} /* namespace s3d */
