/* 
 * Copyright (c) 2012 Brian Kloppenborg
 *
 * If you use this software as part of a scientific publication, please cite as:
 *
 * Kloppenborg, B.; Baron, F. (2012), "SIMTOI: The SImulation and Modeling 
 * Tool for Optical Interferometry" (Version X). 
 * Available from  <https://github.com/bkloppenborg/simtoi>.
 *
 * This file is part of the SImulation and Modeling Tool for Optical 
 * Interferometry (SIMTOI).
 * 
 * SIMTOI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * as published by the Free Software Foundation version 3.
 * 
 * SIMTOI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with SIMTOI.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <QTime>
#include <QtDebug>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <memory>

#include "CCL_GLThread.h"
#include "CGLWidget.h"
#include "CModelList.h"
#include "CModel.h"
#include "CPosition.h"
#include "liboi.hpp"
#include "json/json.h"
#include "textio.hpp"

int CCL_GLThread::count = 0;

using namespace std;

CCL_GLThread::CCL_GLThread(CGLWidget *glWidget, string shader_source_dir, string kernel_source_dir)
	: QThread(), mGLWidget(glWidget)
{
	id = count++;

    mRun = true;
    mIsRunning = false;
    mPermitResize = true;
    mImageWidth = 1;
    mImageHeight = 1;
    mImageDepth = 1;
    mScale = 0.01;	// init to some value > 0.
    mAreaDepth = 100; // +mDepth to -mDepth is the viewing region, in coordinate system units.

    mModelList = new CModelList();

    mKernelSourceDir = kernel_source_dir;
    mCL = NULL;
    mCLInitalized = false;
    // These values come from outside of this class, if they are an array don't delete or free them here.
    mCLDataSet = 0;
    mCLValue = 0;
    mCLArrayValue = NULL;
    mCLArrayN = 0;

    mFBO = 0;
 	mFBO_texture = 0;
 	mFBO_depth = 0;
    mFBO_storage = 0;
	mFBO_storage_texture = 0;
 	mSamples = 4;
}

CCL_GLThread::~CCL_GLThread()
{
	// Free OpenGL memory buffers
	glDeleteFramebuffers(1, &mFBO);
	glDeleteFramebuffers(1, &mFBO_texture);
	glDeleteFramebuffers(1, &mFBO_depth);
	glDeleteFramebuffers(1, &mFBO_storage);
	glDeleteFramebuffers(1, &mFBO_storage_texture);

	delete mCL;
	delete mModelList;
}

/// Appends a model to the model list, importing shaders and features as necessary.
void CCL_GLThread::AddModel(shared_ptr<CModel> model)
{
	// Create the model, load the shader.
	mModelList->AddModel(model);

	EnqueueOperation(GLT_RenderModels);
}

/// Copies the off-screen framebuffer to the on-screen buffer.  To be called only by the thread.
void CCL_GLThread::BlitToScreen()
{
    // Bind back to the default buffer (just in case something didn't do it),
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Blit the application-defined render buffer to the on-screen render buffer.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFBO_storage);
	/// TODO: In QT I don't know what GL_BACK is.  Seems GL_DRAW_FRAMEBUFFER is already set to it though.
    //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_BACK);
    glBlitFramebuffer(0, 0, mImageWidth, mImageHeight, 0, 0, mImageWidth, mImageHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glFinish();
    mGLWidget->swapBuffers();
	CWorkerThread::CheckOpenGLError("CGLThread::BlitToScreen()");
}

/// Blits the input buffer to the out_layer of the output buffer
void CCL_GLThread::BlitToBuffer(GLuint in_buffer, GLuint out_buffer, unsigned int out_layer)
{
	// TODO: Need to figure out how to use the layer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, in_buffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, out_buffer);
	//glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, out_buffer, 0, out_layer);
	glBlitFramebuffer(0, 0, mImageWidth, mImageHeight, 0, 0, mImageWidth, mImageHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glFinish();

  	CWorkerThread::CheckOpenGLError("CGLThread BlitToBuffer");
}

void CCL_GLThread::ClearQueue()
{
	// Clear the queue and reset the semaphore.
	mQueueMutex.lock();

	// Clear the queue
	while (!mQueue.empty())
	{
		mQueue.pop();
	}

	mQueueSemaphore.acquire(mQueueSemaphore.available());
	mQueueMutex.unlock();
}

/// Static function for checking OpenGL errors:
void CWorkerThread::CheckOpenGLError(string function_name)
{
    GLenum status = glGetError(); // Check that status of our generated frame buffer
    // If the frame buffer does not report back as complete
    if (status != 0)
    {
        string errstr =  (const char *) gluErrorString(status);
        printf("Encountered OpenGL Error %x %s\n %s", status, errstr.c_str(), function_name.c_str());
        throw;
    }
}

/// Enqueue an operation for the CCL_GLThread to process.
void CCL_GLThread::EnqueueOperation(CL_GLT_Operations op)
{
	// Lock the queue, append the item, increment the semaphore.
	mQueueMutex.lock();
	mQueue.push(op);
	mQueueMutex.unlock();
	mQueueSemaphore.release();
}

// Exports the simulated and real data for all currently loaded data
// to files whose starting bit is specified by base_filename
void CCL_GLThread::ExportResults(string base_filename)
{
	// TODO: Pull in liboi's new export mechanisim.
}

/// Returns the chi values in output for the specified data set and the current image.
void CCL_GLThread::GetChi(int data_num, float * output, int & n)
{
	mCLDataSet = data_num;
	mCLArrayValue = output;
	mCLArrayN = n;
	EnqueueOperation(CLT_Chi);
	mCLOpSemaphore.acquire();
	n = mCLArrayN;
}

/// Returns the chi2 for the specified data set
double CCL_GLThread::GetChi2(int data_num)
{
	// Set the data number, enqueue the operation and then block until we receive an answer.
	mCLDataSet = data_num;
	EnqueueOperation(CLT_Chi2);
	mCLOpSemaphore.acquire();
	return mCLValue;
}

/// Returns a copy of the ccoifits data loaded in index data_num.
OIDataList CCL_GLThread::GetData(unsigned int data_num)
{
	return mCL->GetData(data_num);
}

/// Returns the average
double CCL_GLThread::GetDataAveJD(int data_num)
{
	if(mCL != NULL)
		return mCL->GetDataAveJD(data_num);

	return 0;
}

/// Returns the flux of the current rendered image
double CCL_GLThread::GetFlux()
{
	EnqueueOperation(CLT_Flux);
	mCLOpSemaphore.acquire();
	return mCLValue;
}

/// Returns the current rendered image, including depth, as a floating point array of size width * height * depth
void CCL_GLThread::GetImage(float * image, unsigned int width, unsigned int height, unsigned int depth)
{
	// Image size must match exactly:
	if(width != mImageWidth || height != mImageHeight || depth != mImageDepth)
		return;

	// Enqueue the copy operation, block until it has completed.
	mCLArrayValue = image;
	EnqueueOperation(CLT_CopyImage);
	mCLOpSemaphore.acquire();
}

/// Returns the chi2 for the specified data set
double CCL_GLThread::GetLogLike(int data_num)
{
	// Set the data number, enqueue the operation and then block until we receive an answer.
	mCLDataSet = data_num;
	EnqueueOperation(CLT_LogLike);
	mCLOpSemaphore.acquire();
	return mCLValue;
}

/// Returns the total number of data points (V2 + T3) in all data sets loaded.
int CCL_GLThread::GetNData()
{
	if(mCL != NULL)
		return mCL->GetNData();

	return 0;
}

/// Returns the total allocation size for all data points
int CCL_GLThread::GetNDataAllocated()
{
	if(mCL != NULL)
		return mCL->GetNDataAllocated();

	return 0;
}

/// Returns the data allocation size for the data_num entry, returns zero if data_num is invalid.
int CCL_GLThread::GetNDataAllocated(int data_num)
{
	if(mCL != NULL)
		return mCL->GetNDataAllocated(data_num);

	return 0;
}

/// Returns the total number of data sets.
int CCL_GLThread::GetNDataSets()
{
	if(mCL != NULL)
		return mCL->GetNDataSets();

	return 0;
}

int CCL_GLThread::GetNT3(int data_num)
{
	if(mCL != NULL)
		return mCL->GetNT3(data_num);

	return 0;
}

int CCL_GLThread::GetNV2(int data_num)
{
	if(mCL != NULL)
		return mCL->GetNV2(data_num);

	return 0;
}


/// Get the next operation from the queue.  This is a blocking function.
CL_GLT_Operations CCL_GLThread::GetNextOperation(void)
{
	// First try to get access to the semaphore.  This is a blocking call if the queue is empty.
	mQueueSemaphore.acquire();
	// Now lock the queue, pull off the top item, pop it from the queue, and return.
	mQueueMutex.lock();
	CL_GLT_Operations tmp = mQueue.top();
	mQueue.pop();
	mQueueMutex.unlock();
	return tmp;
}

void CCL_GLThread::InitFrameBuffers(void)
{
	InitMultisampleRenderBuffer();
	InitStorageBuffer();
}

void CCL_GLThread::InitMultisampleRenderBuffer(void)
{
	glGenFramebuffers(1, &mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

	glGenRenderbuffers(1, &mFBO_texture);
	glBindRenderbuffer(GL_RENDERBUFFER, mFBO_texture);
	// Create a 2D multisample texture
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, mSamples, GL_RGBA32F, mImageWidth, mImageHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mFBO_texture);

	glGenRenderbuffers(1, &mFBO_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, mFBO_depth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, mSamples, GL_DEPTH_COMPONENT, mImageWidth, mImageHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mFBO_depth);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    // Check that status of our generated frame buffer
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
    	const GLubyte * errStr = gluErrorString(status);
        printf("Couldn't create multisample frame buffer: %x %s\n", status, (char*)errStr);
        delete errStr;
        exit(0); // Exit the application
    }

    GLint bufs;
    GLint samples;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &bufs);
    glGetIntegerv(GL_SAMPLES, &samples);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind our frame buffer
}

void CCL_GLThread::InitStorageBuffer(void)
{
    glGenTextures(1, &mFBO_storage_texture); // Generate one texture
    glBindTexture(GL_TEXTURE_2D, mFBO_storage_texture); // Bind the texture mFBOtexture

    // Create the texture in red channel only 8-bit (256 levels of gray) in GL_BYTE (CL_UNORM_INT8) format.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mImageWidth, mImageHeight, 0, GL_RED, GL_FLOAT, NULL);
    // Enable this one for alpha blending:
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, NULL);
    // These other formats might work, check that GL_BYTE is still correct for the higher precision.
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, mWidth, mHeight, 0, GL_RED, GL_BYTE, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32, mWidth, mHeight, 0, GL_RED, GL_BYTE, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, mWidth, mHeight, 0, GL_RED, CL_HALF_FLOAT, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mWidth, mHeight, 0, GL_RED, GL_FLOAT, NULL);


    // Setup the basic texture parameters
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &mFBO_storage); // Generate one frame buffer and store the ID in mFBO
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO_storage); // Bind our frame buffer

    // Attach the depth and texture buffer to the frame buffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFBO_storage_texture, 0);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mFBO_depth);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    // Check that status of our generated frame buffer
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
    	const GLubyte * errStr = gluErrorString(status);
        printf("Couldn't create storage frame buffer: %x %s\n", status, (char*)errStr);
        delete errStr;
        exit(0); // Exit the application
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind our frame buffer
}


/// Loads data.
int CCL_GLThread::LoadData(string filename)
{
	mCLString = filename;
	EnqueueOperation(CLT_DataLoadFromString);
	mCLOpSemaphore.acquire();
}

/// Loads data to the OpenCL device. Returns the data id (>= 0) on success, -1 on failure.
int CCL_GLThread::LoadData(const OIDataList & data)
{
	mCLDataList = data;
	EnqueueOperation(CLT_DataLoadFromList);
	mCLOpSemaphore.acquire();
	return mCL->GetNData();
}

/// Opens a save file
void CCL_GLThread::Open(string filename)
{
	Json::Reader reader;
	Json::Value input;
	string file_contents = ReadFile(filename, "Could not read model save file.");
	bool parsingSuccessful = reader.parse(file_contents, input);
	if(parsingSuccessful)
		mModelList->Restore(input);

	EnqueueOperation(GLT_RenderModels);
}

void CCL_GLThread::RemoveData(int data_num)
{
	mCLDataSet = data_num;
	EnqueueOperation(CLT_DataRemove);
	mCLOpSemaphore.acquire();
}

/// Replaces the data set in ID old_data_id with new_data
void CCL_GLThread::ReplaceData(unsigned int old_data_id, const OIDataList & new_data)
{
	mCLException = NULL;

	mCLDataSet = old_data_id;
	mCLDataList = new_data;
	EnqueueOperation(CLT_DataReplace);
	mCLOpSemaphore.acquire();

	// Pass any exceptions on to the calling thread
	if(mCLException)
		rethrow_exception(mCLException);
}

/// Resets any OpenGL errors by looping.
void CCL_GLThread::ResetGLError()
{
    while (glGetError() != GL_NO_ERROR) {};
}

/// Resize the window.  Normally called from QT
void CCL_GLThread::resizeViewport(const QSize &size)
{
    resizeViewport(size.width(), size.height());
}

/// Resize the window.  Called from external applications.
void CCL_GLThread::resizeViewport(int width, int height)
{
	if(mPermitResize)
	{
		mImageWidth = width;
		mImageHeight = height;
		EnqueueOperation(GLT_Resize);
	}
}   

/// Run the thread.
void CCL_GLThread::run()
{
	// Claim the OpenGL context.
    mGLWidget->makeCurrent();

	// ########
	// OpenGL initialization
	// ########
	glClearColor(0.0, 0.0, 0.0, 0.0);
	// Set to flat (non-interpolated) shading:
	glShadeModel(GL_FLAT);
	glDisable(GL_DITHER);
	glEnable(GL_DEPTH_TEST);    // enable the Z-buffer depth testing

	// Enable alpha blending:
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable multisample anti-aliasing.
	glEnable(GL_MULTISAMPLE);
	//glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);

	// Now setup the projection system to be orthographic
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Note, the coordinates here are in object-space, not coordinate space.
	double half_width = mImageWidth * mScale;
	glOrtho(-half_width, half_width, -half_width, half_width, -mAreaDepth, mAreaDepth);

    // Init the off-screen frame buffers.
    InitFrameBuffers();

    // Start the thread
    mRun = true;
    EnqueueOperation(GLT_RenderModels);
    //EnqueueOperation(GLT_BlitToScreen);
    CL_GLT_Operations op;

	CWorkerThread::CheckOpenGLError("Error occurred during GL Thread Initialization.");

	// ########
	// OpenCL initialization
	// ########
	mCL = new CLibOI(CL_DEVICE_TYPE_GPU);
	mCL->SetImageSource(mFBO_storage_texture, LibOIEnums::OPENGL_TEXTUREBUFFER);
	mCL->SetKernelSourcePath(mKernelSourceDir);

	// ########
	// Start the thread main body. Note, execution is driven by operations
	// inserted into the queue via. CCL_GLThread::EnqueueOperation(...)
	// commands.
	// ########

    // Indicate that the thread is running
	mIsRunning = true;
    while (mRun)
    {
        op = GetNextOperation();

        // NOTE: Resize and Render cascade.
        switch(op)
        {
        case GLT_Animate:
//         	mModelList->IncrementTime();
         	QThread::msleep(40);
         	EnqueueOperation(GLT_RenderModels);
         	EnqueueOperation(GLT_Animate);
         	break;

        case GLT_Resize:
        	// Resize the screen, then cascade to a render and a blit.
            glViewport(0, 0, mImageWidth, mImageHeight);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            half_width = mImageWidth * mScale / 2;
			glOrtho(-half_width, half_width, -half_width, half_width, -mAreaDepth, mAreaDepth);
            glMatrixMode(GL_MODELVIEW);
        	CWorkerThread::CheckOpenGLError("CGLThread GLT_Resize");
        	// Now tell OpenCL about the image (depth = 1 because we have only one layer)
        	mCL->SetImageInfo(mImageWidth, mImageHeight, 1, double(mScale));
        	mPermitResize = false;

        default:
        case GLT_RenderModels:
            // Render the models, then cascade to a blit to screen.
        	CWorkerThread::CheckOpenGLError("CGLThread GLT_RenderModels Entry");
            mModelList->Render(mFBO, mImageWidth, mImageHeight);
            BlitToBuffer(mFBO, mFBO_storage, 0);

     	case GLT_BlitToScreen:
			BlitToScreen();
        	CWorkerThread::CheckOpenGLError("CGLThread GLT_BlitToScreen");
			break;

        case GLT_Stop:
            mRun = false;
            break;

        case GLT_AnimateStop:
        	ClearQueue();
        	EnqueueOperation(GLT_RenderModels);
        	break;

        case CLT_DataLoadFromString:
        	mCL->LoadData(mCLString);
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_DataLoadFromList:
        	mCL->LoadData(mCLDataList);
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_DataRemove:
        	mCL->RemoveData(mCLDataSet);
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_DataReplace:
        	try
        	{
        		mCL->ReplaceData(mCLDataSet, mCLDataList);
        	}
        	catch(...)
        	{
        		mCLException = current_exception();
        	}
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_Chi:
        	// Copy the image to the buffer, compute the chi values, and initiate a copy to the
        	// local value.
        	mCL->CopyImageToBuffer(0);
        	mCL->ImageToChi(mCLDataSet, mCLArrayValue, mCLArrayN);
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_Chi2:
        	// Copy the image into the buffer, compute the chi2, set the value, release the operation semaphore.
        	// TODO: Note the spectral data will need something special here.
        	mCL->CopyImageToBuffer(0);
        	mCLValue = mCL->ImageToChi2(mCLDataSet);
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_Flux:
        	// Copy the image to the buffer, compute the chi values, and initiate a copy to the
        	// local value.
        	mCL->CopyImageToBuffer(0);
        	mCLValue = mCL->TotalFlux(true);
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_Init:
        	// Init all LibOI routines
        	mCL->Init();
        	mCLInitalized = true;
        	break;

        case CLT_LogLike:
        	// Copy the image into the buffer, compute the chi2, set the value, release the operation semaphore.
        	// TODO: Note the spectral data will need something special here.
        	mCL->CopyImageToBuffer(0);
        	mCLValue = mCL->ImageToLogLike(mCLDataSet);
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_Tests:
        	// Runs the LibOI test sequence on the zeroth data set
        	mCL->CopyImageToBuffer(0);
        	mCL->RunVerification(0);
        	break;

        case CLT_CopyImage:
        	// Copy the current OpenCL image into the mCLArrayValue CPU buffer
        	mCL->CopyImageToBuffer(0);
        	mCL->ExportImage(mCLArrayValue, mImageWidth, mImageHeight, mImageDepth);
        	mCLOpSemaphore.release(1);
        	break;

        case CLT_SaveImage:
        	// Copy the current OpenCL image into the mCLArrayValue CPU buffer
        	mCL->CopyImageToBuffer(0);
        	mCL->SaveImage(mCLString);
        	mCLOpSemaphore.release(1);
        	break;

//        case CLT_GetData:
//        	mCL->CopyImageToBuffer(0);
//        	mCL->GetSimulatedData(mCLDataSet, mCLArrayValue, mCLArrayN);
//        	mCLOpSemaphore.release(1);
//        	break;

        case CLT_GetChi2_Elements:
        	mCL->CopyImageToBuffer(0);
        	mCL->ImageToChi2(mCLDataSet, mCLArrayValue, mCLArrayN);
        	mCLOpSemaphore.release(1);
        	break;
        }
    }

    // The thread is no longer running
    mIsRunning = false;
}

/// Saves the list of models and their values to the specified location
/// in the JSON file format.
void CCL_GLThread::Save(string filename)
{
	Json::StyledStreamWriter writer;
	Json::Value output = mModelList->Serialize();
	std::ofstream outfile(filename.c_str());
	writer.write(outfile, output);
}

void CCL_GLThread::SaveImage(string filename)
{
	// Enqueue the copy operation, block until it has completed.
	mCLString = filename;
	EnqueueOperation(CLT_SaveImage);
	mCLOpSemaphore.acquire();
}

/// Sets the scale for the model.
void CCL_GLThread::SetFreeParameters(double * params, unsigned int n_params, bool scale_params)
{
	mModelList->SetFreeParameters(params, n_params, scale_params);
	EnqueueOperation(GLT_RenderModels);
}

/// Sets the scale for the model.
void CCL_GLThread::SetScale(double scale)
{
	if(scale > 0)
		mScale = scale;
}

void CCL_GLThread::SetTime(double t)
{
	mModelList->SetTime(t);
}

void CCL_GLThread::SetTimestep(double dt)
{
//	mModelList->SetTimestep(dt);
}

/// Stop the thread.
void CCL_GLThread::stop()
{
    EnqueueOperation(GLT_Stop);
}
