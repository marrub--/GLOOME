#ifndef GL_STEREO3D_H_
#define GL_STEREO3D_H_

#include <vector>
#include "gl/system/gl_system.h"


namespace s3d {


class Viewport
{
public:
	int x, y;
	int width, height;
};


class EyePose 
{
public:
	EyePose() {}
	virtual ~EyePose() {}
	virtual void GetProjection(float fov, float aspectRatio, float fovRatio, GLdouble outMatrix[4][4]) const;
	virtual Viewport GetViewport(const Viewport& fullViewport) const;
	virtual void GetView(
		float viewx, float viewy, float viewz,
		float roll, float pitch, float yaw,
		float outViewPos[3], float outRollPitchYaw[3]) const;
	virtual void SetUp() const {};
	virtual void TearDown() const {};
};


class Stereo3DMode
{
public:
	typedef const EyePose * const_iterator;

	static const Stereo3DMode& getCurrentMode();
	static void setCurrentMode(const Stereo3DMode& mode);

	Stereo3DMode();
	virtual ~Stereo3DMode();
	virtual const_iterator begin() const = 0;
	virtual const_iterator end() const = 0;
	virtual void SetUp() const {};
	virtual void TearDown() const {};

protected:
	static Stereo3DMode const * currentStereo3DMode;
};


/**
*  Ordinary non-3D rendering
*/
class Mono : public Stereo3DMode
{
public:
	static const Mono& getInstance();

	virtual const_iterator begin() const { return &theEye; }
	virtual const_iterator end() const { return (&theEye) + 1; }

protected:
	Mono();
	EyePose theEye;
};


} /* namespace st3d */


#endif /* GL_STEREO3D_H_ */
