/*
 * CModelDisk.h
 *
 *  Created on: Feb 23, 2012
 *      Author: bkloppenborg
 *
 *  Implements a simple disk model which has two parameters: diameter and height.
 *  When rendered, this model is a cylinder.  Inheriting classes should call
 *  the CModelDisk(int) function and start numbering additional parameters at 3.
 */

#ifndef CMODELDISK_H_
#define CMODELDISK_H_

#include "CModel.h"

class CModelDisk : public CModel
{
protected:
	int mSlices;
	int mStacks;
	double * mSinT;
	double * mCosT;
	double mZeroThreshold;

public:
	CModelDisk();
	CModelDisk(int additional_params);
	virtual ~CModelDisk();

	void Draw();
	void DrawDisk(double radius, double height);
	void DrawSides(double radius, double height);

	double GetRadius(double height, double rim_radius);

	void InitMembers();

	void Render(GLuint framebuffer_object, int width, int height);

	double Transparency(double half_height, double at_z);
};

#endif /* CMODELDISK_GAUSS_H_ */