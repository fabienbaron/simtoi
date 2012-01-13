
#ifndef GLTHREAD
#define GLTHREAD

#include <QObject>
#include <QThread>
#include <QSize>
#include <QMutex>
#include <QSemaphore>
#include <string>
#include <queue>

#include <GL/gl.h>
#include <GL/glu.h>

class CGLWidget;
class CModelList;
class CGLShaderList;
class CModel;

using namespace std;

// A list of operations permitted.
enum GLT_Operations
{
	GLT_BlitToScreen,
	GLT_Resize,
	GLT_RenderModels,
	GLT_Stop
};

/// A quick class for making priority queue comparisions.  Used in CGLThread
class GLQueueComparision
{
public:
	GLQueueComparision() {};

	bool operator() (const GLT_Operations& lhs, const GLT_Operations&rhs) const
	{
		if(rhs == GLT_Stop)
			return true;

		return false;
	}
};


class CGLThread : public QThread {
    Q_OBJECT

protected:
	priority_queue<GLT_Operations, vector<GLT_Operations>, GLQueueComparision> mQueue;
    QMutex mQueueMutex;
    QSemaphore mQueueSemaphore;
    CGLWidget * mGLWidget;
    CModelList * mModelList;
    CGLShaderList * mShaderList;
    bool mPermitResize;
    bool mResizeInProgress;
    int mWidth;
    int mHeight;
    double mScale;

    //QGLFramebufferObject * mFBO
	GLuint mFBO;
	GLuint mFBO_texture;
	GLuint mFBO_depth;

	bool mRun;

    bool doResize;
    float rotAngle;
    int id;
    static int count;

public:
    CGLThread(CGLWidget * glWidget, string shader_source_dir);
    ~CGLThread();

    void AddModel(CModel * model);

protected:
    void BlitToScreen();
public:
    static void CheckOpenGLError(string function_name);

public:
    void EnqueueOperation(GLT_Operations op);
    GLT_Operations GetNextOperation(void);

protected:
    void InitFrameBuffer(void);
    void InitFrameBufferDepthBuffer(void);
    void InitFrameBufferTexture(void);

private:
    void glDrawTriangle();

public:
    static void ResetGLError();
    void resizeViewport(const QSize &size);
    void resizeViewport(int width, int height);
    void run();

    void SetScale(double scale);
    void stop();

};
    
#endif
