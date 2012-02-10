/*
 * CCModelList.cpp
 *
 *  Created on: Nov 8, 2011
 *      Author: bkloppenborg
 */

#include "CModelList.h"
#include "CCL_GLThread.h"
#include "CModel.h"
#include "CModelSphere.h"
#include "CModelCylinder.h"
#include "CPosition.h"

using namespace std;

CModelList::CModelList()
{
	mTime = 0;
	mTimestep = 0;
}

CModelList::~CModelList()
{

}

/// Creates a new model, appends it to the model list and returns a pointer to it.
CModel * CModelList::AddNewModel(ModelTypes model_id)
{
	CModel * tmp;
	switch(model_id)
	{
	case CYLINDER:
		tmp = new CModelCylinder();
		break;

	case SPHERE:
	default:
		tmp = new CModelSphere();
		break;
	}

	this->Append(tmp);
	return mList.back();
}

/// Returns the total number of free parameters in the models
int CModelList::GetNFreeParameters()
{
    int n = 0;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	n +=(*it)->GetTotalFreeParameters();
    }

    return n;
}

void CModelList::GetAllParameters(float * params, int n_params)
{
    int n = 0;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	(*it)->GetAllParameters(params + n, n_params - n);
    	n += (*it)->GetTotalFreeParameters();
    }
}

vector< pair<float, float> > CModelList::GetFreeParamMinMaxes()
{
    vector< pair<float, float> > tmp1;
    vector< pair<float, float> > tmp2;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	tmp2 = (*it)->GetFreeParamMinMaxes();
    	tmp1.insert( tmp1.end(), tmp2.begin(), tmp2.end() );
    }

    return tmp1;
}

/// Gets the values for all of the free parameters.
void CModelList::GetFreeParameters(float * params, int n_params)
{
    int n = 0;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	(*it)->GetFreeParameters(params + n, n_params - n);
    	n += (*it)->GetTotalFreeParameters();
    }
}

/// Gets the values for all of the free parameters.
void CModelList::GetFreeParametersScaled(float * params, int n_params)
{
    int n = 0;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	(*it)->GetFreeParametersScaled(params + n, n_params - n);
    	n += (*it)->GetTotalFreeParameters();
    }
}

/// Returns a vector of string containing the parameter names.
vector<string> CModelList::GetFreeParamNames()
{
    vector<string> tmp1;
    vector<string> tmp2;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	tmp2 = (*it)->GetFreeParameterNames();
    	tmp1.insert( tmp1.end(), tmp2.begin(), tmp2.end() );
    }

    return tmp1;
}

/// Returns a pair of model names, and their enumerated types
vector< pair<CModelList::ModelTypes, string> > CModelList::GetTypes(void)
{
	vector< pair<ModelTypes, string> > tmp;
	tmp.push_back(pair<ModelTypes, string> (CModelList::SPHERE, "Sphere"));
	tmp.push_back(pair<ModelTypes, string> (CModelList::CYLINDER, "Cylinder"));

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


void CModelList::SetFreeParameters(float * params, int n_params)
{
    int n = 0;

    // Now call render on all of the models:
    for(vector<CModel*>::iterator it = mList.begin(); it != mList.end(); ++it)
    {
    	(*it)->SetFreeParameters(params + n, n_params - n);
    	n += (*it)->GetTotalFreeParameters();
    }
}

void CModelList::SetPositionType(int model_id, CPosition::PositionTypes pos_type)
{
	if(model_id < mList.size())
		mList[model_id]->SetPositionType(pos_type);
}

void CModelList::SetShader(int model_id, CGLShaderWrapper * shader)
{
	if(model_id < mList.size())
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
