#ifndef GL_STEREO_LEFTRIGHT_H_
#define GL_STEREO_LEFTRIGHT_H_

#include "gl_stereo3d.h"

namespace s3d {

class ShiftedEyePose : public EyePose
{
public:
	ShiftedEyePose(float shift);
	float getShift() const { return shift; }
	void setShift(float shift) { this->shift = shift; }
protected:
	float shift;
};

class LeftEyePose : public ShiftedEyePose
{
public:
	LeftEyePose(float ipd) : ShiftedEyePose(-0.5*ipd) {}
	float getIpd() const { return -2.0*shift; }
	void setIpd(float ipd) { setShift(-0.5*ipd); }
};

class RightEyePose : public ShiftedEyePose
{
public:
	RightEyePose(float ipd) : ShiftedEyePose(+0.5*ipd) {}
	float getIpd() const { return +2.0*shift; }
	void setIpd(float ipd) { setShift(+0.5*ipd); }
};

class LeftEyeView : public Stereo3DMode
{
	LeftEyeView(float ipd);
	float getIpd() const { return eye.getIpd(); }
	void setIpd(float ipd) { eye.setIpd(ipd); }
protected:
	LeftEyePose eye;
};

} /* namespace s3d */

#endif /* GL_STEREO_LEFTRIGHT_H_ */
