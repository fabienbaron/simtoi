
#ifndef GLWIDGET
#define GLWIDGET

#include <QObject>
#include <QThread>
#include <QGLWidget>
#include <QResizeEvent>
#include <QtDebug>
#include <QStandardItemModel>
#include <utility>
#include <vector>

#include "CCL_GLThread.h"
#include "CMinimizerThread.h"
#include "CLibOI.h"
#include "CModelList.h"
#include "CMinimizer.h"

class CModel;
class CTreeModel;

class CGLWidget : public QGLWidget
{
    Q_OBJECT
    
protected:
    CCL_GLThread mGLT;
    CMinimizerThread mMinThread;
    QStandardItemModel * mOpenFileModel;
    CTreeModel * mTreeModel;
    static QGLFormat mFormat;

public:
    CGLWidget(QWidget *widget_parent, string shader_source_dir, string cl_kernel_dir);
    ~CGLWidget();

    void AddModel(CModelList::ModelTypes model);
protected:
    void closeEvent(QCloseEvent *evt);
public:

    void EnqueueOperation(CL_GLT_Operations op);
	vector< pair<CGLShaderList::ShaderTypes, string> > GetShaderNames(void) { return mGLT.GetShaderNames(); };

protected:
	void paintEvent(QPaintEvent * );

    void resizeEvent(QResizeEvent *evt);

public:

    vector< pair<CModelList::ModelTypes, string> > GetModelTypes() { return mGLT.GetModelTypes(); };
    vector< pair<CGLShaderList::ShaderTypes, string> > GetShaderTypes() { return mGLT.GetShaderTypes(); };
//    vector< pair<int, string> > GetPositionTypes() { return mGLT.GetPositionTypes(); };

    double GetFlux() { return mGLT.GetFlux(); };
    void GetImage(float * image, unsigned int width, unsigned int height, unsigned int depth) { mGLT.GetImage(image, width, height, depth); };
    unsigned int GetImageDepth() { return mGLT.GetImageDepth(); };
    unsigned int GetImageHeight() { return mGLT.GetImageHeight(); };
    unsigned int GetImageWidth() { return mGLT.GetImageWidth(); };
    float GetImageScale() { return mGLT.GetScale(); };
    int GetNData() { return mGLT.GetNData(); };
    int GetNDataSets() { return mGLT.GetNDataSets(); };
    double GetDataAveJD(int data_num) { return mGLT.GetDataAveJD(data_num); };
    int GetNModels() { return mGLT.GetModelList()->size(); };
    CModelList * GetModelList() { return mGLT.GetModelList(); };
    QStandardItemModel * GetOpenFileModel() { return mOpenFileModel; };
    double GetScale() { return mGLT.GetScale(); };
    CTreeModel * GetTreeModel() { return mTreeModel; };


    void LoadData(string filename) { mGLT.LoadData(filename); };
    void LoadMinimizer(CMinimizer::MinimizerTypes minimizer_type);
protected:
    void LoadParameters(QStandardItem * parent, CParameters * parameters);
    QList<QStandardItem *> LoadParametersHeader(QString name, CParameters * param_base);

public:
    void Open(string filename);
    bool OpenCLInitialized() { return mGLT.OpenCLInitialized(); };

    void RunMinimizer();
protected:
    void RebuildTree();
public:
    void RemoveData(int data_num) { mGLT.RemoveData(data_num); };

public:
    void Save(string location) { mGLT.Save(location); };
    void SaveImage(string filename) { mGLT.SaveImage(filename); };
    void SetFreeParameters(double * params, int n_params, bool scale_params);
    void SetScale(double scale);
    void SetShader(int model_id, CGLShaderList::ShaderTypes shader);
    void SetPositionType(int model_id, CPosition::PositionTypes pos_type);
    void SetTime(double t) { mGLT.SetTime(t); };
    void SetTimestep(double dt) { mGLT.SetTimestep(dt); };
    void StopMinimizer();

    void startRendering();
    void stopRendering();
};

#endif
