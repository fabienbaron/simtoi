 /*
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
 *
 * Copyright (c) 2012, 2014 Brian Kloppenborg
 */
 
#ifndef C_WORKER_THREAD
#define C_WORKER_THREAD

#include <QObject>
#include <QThread>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QSemaphore>
#include <QSize>
#include <QGLFramebufferObject>
#include <valarray>
#include <memory>
#include <queue>
#include "json/json.h"
#include "CDataInfo.h"
#include "CTask.h"

#include "OpenGL.h" // OpenGL includes, plus several workarounds for various OSes

// OpenGL mathematics library:
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using namespace std;

class CGLWidget;
class CTaskList;
typedef shared_ptr<CTaskList> CTaskListPtr;
class CModel;
typedef shared_ptr<CModel> CModelPtr;
class CModelList;
typedef shared_ptr<CModelList> CModelListPtr;
class COpenCL;
typedef shared_ptr<COpenCL> COpenCLPtr;

enum WorkerOperations
{
	BOOTSTRAP_NEXT,
	EXPORT,
	GET_CHI,
	GET_UNCERTAINTIES,
	OPEN_DATA,
	RENDER,
	SET_TIME,
	SET_WAVELENGTH,
	STOP
};

/// A quick class for making priority queue comparisons.  Used for CCL_GLThread, mQueue
class WorkerQueueComparision
{
public:
	WorkerQueueComparision() {};

	bool operator() (const WorkerOperations & lhs, const WorkerOperations & rhs) const
	{
		if(rhs == STOP)
			return true;

		return false;
	}
};

class CWorkerThread : public QThread
{
    Q_OBJECT
protected:
    // Datamembers for the OpenGL context
    CGLWidget * mGLWidget;	///< Managed elsewhere, do not delete.
    bool mGLFloatSupported;
    GLint mGLRenderBufferFormat;
    GLint mGLStorageBufferFormat;
    GLenum mGLPixelDataType;

    unsigned int mImageDepth;
    unsigned int mImageHeight;
    double mImageScale;
    unsigned int mImageWidth;
    unsigned int mImageSamples;
    glm::mat4 mView;

    // Off-screen framebuffer (this matches the buffer created by CreateGLBuffer)
    // All rendering from the UI happens in these buffers. Results are blitted to screen.
	QGLFramebufferObject * mFBO_render;

    // OpenCL
    COpenCLPtr mOpenCL;

    // other data members:
    shared_ptr<CTaskList> mTaskList;
    shared_ptr<CModelList> mModelList;
    QString mExeFolder;

    // Queue:
	priority_queue<WorkerOperations, vector<WorkerOperations>, WorkerQueueComparision> mTaskQueue;
	QMutex mTaskMutex;			// For adding/removing items from the operation queue
	QSemaphore mTaskSemaphore;	// For blocking calling thread while operation finishes

	bool mRun;
	QMutex mWorkerMutex;			// Lock to have exclusive access to this object (all calls from external threads do this)
	QSemaphore mWorkerSemaphore;	// Acquire if a read/write operation is enqueued.

	// Temporary storage locations
	double * mTempArray;	///< External memory. Don't allocate/deallocate.
	unsigned int mTempArraySize;
	string mTempString;
	double mTempDouble;
	unsigned int mTempUint;
	CDataInfo mTempDataInfo;

public:
    CWorkerThread(CGLWidget * glWidget, QString exe_folder);
    virtual ~CWorkerThread();

public:
	void addModel(CModelPtr model);
	CModelPtr getModel(unsigned int model_index);
	void replaceModel(unsigned int model_index, CModelPtr new_model);
	void removeModel(unsigned int model_index);

	CDataInfo addData(string filename);
	void removeData(unsigned int data_id);

    void AllocateBuffer();

public:
    void BlitToBuffer(QGLFramebufferObject * source, QGLFramebufferObject * target);
    void BlitToScreen(QGLFramebufferObject * input);

    void BlitToBuffer(GLuint in_buffer, GLuint out_buffer);
    void BlitToScreen(GLuint FBO);
    void BootstrapNext(unsigned int maxBootstrapFailures);

//    static void CheckOpenGLError(string function_name);
protected:
    void ClearQueue();
public:
    QGLFramebufferObject * CreateMAARenderbuffer();
    QGLFramebufferObject * CreateStorageBuffer();

    void CreateGLBuffer(GLuint & FBO, GLuint & FBO_texture, GLuint & FBO_depth, GLuint & FBO_storage, GLuint & FBO_storage_texture, int n_layers);
    void CreateGLBuffer(GLuint & FBO, GLuint & FBO_texture, GLuint & FBO_depth, GLuint & FBO_storage, GLuint & FBO_storage_texture);
    void CreateGLMultisampleRenderBuffer(unsigned int width, unsigned int height, unsigned int samples,
    		GLuint & FBO, GLuint & FBO_texture, GLuint & FBO_depth);
    void CreateGLStorageBuffer(unsigned int width, unsigned int height, unsigned int depth, GLuint & FBO_storage, GLuint & FBO_storage_texture);

    void Enqueue(WorkerOperations op);
public:
    void ExportResults(QString save_folder);

    double GetTime();
    void GetChi(double * chi, unsigned int size);
    unsigned int GetDataSize();
    QStringList GetFileFilters();
    CModelListPtr GetModelList() { return mModelList; };
    WorkerOperations GetNextOperation(void);
    CTaskListPtr GetTaskList() { return mTaskList; };
    void GetUncertainties(double * uncertainties, unsigned int size);

    unsigned int GetImageDepth() { return mImageDepth; };
    unsigned int GetImageHeight() { return mImageHeight; };
    unsigned int GetImageWidth() { return mImageWidth; };
    double GetImageScale() { return mImageScale; };
    int GetNDataFiles();
    COpenCLPtr GetOpenCL() { return mOpenCL; };
    glm::mat4 GetView() { return mView; };

	GLint glRenderBufferFormat() { return mGLRenderBufferFormat; }
	GLint glRenderStorageFormat() { return mGLStorageBufferFormat; }
	GLenum glPixelDataFormat() { return mGLPixelDataType; }

//    void OpenData(string filename);

    void Render();
public:
    void Restore(Json::Value input);
    void run();

    void SetScale(double scale);
    void SetSize(unsigned int width, unsigned int height);
    void SetTime(double time);
    void SetWavelength(double wavelength);
    Json::Value Serialize();
    void stop();
protected:
    void SwapBuffers();

// Signals and slots
signals:
	void dataAdded(CDataInfo info);
	void glContextWarning(string message);
};
    
#endif // C_WORKER_THREAD
