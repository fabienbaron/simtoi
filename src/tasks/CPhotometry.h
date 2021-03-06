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
 * Copyright (c) 2013 Brian Kloppenborg
 */

#ifndef CPHOTOMETRY_H_
#define CPHOTOMETRY_H_

#include "CTask.h"
#include "liboi.hpp"

#include <string>

using namespace std;
using namespace liboi;

class CPhotometricDataPoint
{
public:
	double jd;
	double mag;
	double mag_err;
	double wavelength;
};
typedef shared_ptr<CPhotometricDataPoint> CPhotometricDataPointPtr;

class CPhotometricDataFile
{
public:
	double mJDStart;
	double mJDEnd;
	double mJDMean;
	double mWavelengthMean;

	string mFilename;		///< The full filename including path and extension
	string mFilenameShort;	///< The filename less the path and extension.

	vector<CPhotometricDataPointPtr> data;

	unsigned int GetNData() { return data.size(); };
};
typedef shared_ptr<CPhotometricDataFile> CPhotometricDataFilePtr;

class CPhotometry: public CTask
{
protected:
	string mFilenameNoExtension;

protected:
	QGLFramebufferObject * mFBO_render;
	QGLFramebufferObject * mFBO_storage;

	CLibOI * mLibOI;
	bool mLibOIInitialized;

	vector<CPhotometricDataFilePtr> mData;

public:
	CPhotometry(CWorkerThread * WorkerThread);
	virtual ~CPhotometry();

	virtual void BootstrapNext(unsigned int maxBootstrapFailures);

	static CTaskPtr Create(CWorkerThread * worker);
	void clearData();

	virtual void Export(string folder_name);

	virtual void GetChi(double * residuals, unsigned int size);
	CDataInfo getDataInfo();
	CDataInfo getDataInfo(CPhotometricDataFilePtr data_file);
	virtual unsigned int GetNData();
	virtual int GetNDataFiles();
	virtual void GetUncertainties(double * residuals, unsigned int size);
	double GetWavelength(const string & wavelength_or_band);

	void InitBuffers();
	virtual void InitGL();
	virtual void InitCL();

	CDataInfo OpenData(string filename);

	void RemoveData(unsigned int data_index);

	double SimulatePhotometry(CModelListPtr model_list, CPhotometricDataPointPtr data_point);
};

#endif /* CPHOTOMETRY_H_ */
