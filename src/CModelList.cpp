/*
 * CCModelList.cpp
 *
 *  Created on: Nov 8, 2011
 *      Author: bkloppenborg
 */

#include "CModelList.h"
#include "CGLThread.h"
#include "CModel.h"
#include "CModelSphere.h"
#include "CModelCylinder.h"
#include "CPosition.h"

using namespace std;

CModelList::CModelList()
{

}

CModelList::~CModelList()
{

}

/// Creates a new model, appends it to the model list and returns a pointer to it.
CModel * CModelList::AddNewModel(eModels model_id)
{
	CModel * tmp;
	switch(model_id)
	{
	case MDL_CYLINDER:
		tmp = new CModelCylinder();
		break;

	case MDL_SPHERE:
	default:
		tmp = new CModelSphere();
		break;
	}

	this->Append(tmp);
	return mList.back();
}

void CModelList::GetParameters(float * params, int n_params)
{
    int n = 0;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	(*it)->GetAllParameters(params + n, n_params - n);
    	n += (*it)->GetTotalFreeParameters();
    }
}

/// Returns a pair of model names, and their enumerated types
vector< pair<eModels, string> > CModelList::GetList_AllModels(void)
{
	vector< pair<eModels, string> > tmp;
	tmp.push_back(pair<eModels, string> (MDL_SPHERE, "Sphere"));
	tmp.push_back(pair<eModels, string> (MDL_CYLINDER, "Cylinder"));

	return tmp;
}

/// Increments the time by the set timestep value.
void CModelList::IncrementTime()
{
	SetTime(mTime + mTimestep);
}

// Render the image to the specified OpenGL framebuffer object.
void CModelList::Render(GLuint fbo, int width, int height)
{
	// First clear the buffer.
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor (0.0f, 0.0f, 0.0f, 0.0f); // Set the clear color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the depth and color buffers

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	(*it)->Render(fbo, width, height);
    }

    // Bind back to the default framebuffer and let OpenGL finish:
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glFlush();
    glFinish();
}


void CModelList::SetParameters(float * params, int n_params)
{
    int n = 0;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	(*it)->SetAllParameters(params + n, n_params - n);
    	n += (*it)->GetTotalFreeParameters();
    }
}

void CModelList::SetPositionType(int model_id, ePositionTypes pos_type)
{
	mList[model_id]->SetPositionType(pos_type);
}

void CModelList::SetShader(int model_id, CGLShaderWrapper * shader)
{
	mList[model_id]->SetShader(shader);
}

/// Sets the time for all of the models
/// Note, some modes don't care about time
void CModelList::SetTime(double t)
{
	mTime = t;
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	(*it)->SetTime(mTime);
    }
}

/// Sets the time increment (for use with animation).
void CModelList::SetTimestep(double dt)
{
	if(dt > 0)
		mTimestep = dt;
}
