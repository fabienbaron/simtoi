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

#ifndef CTASK_H_
#define CTASK_H_

#include <string>
#include <vector>
#include <valarray>

#include "CWorkerThread.h"
#include "CDataInfo.h"

class CTask;
typedef shared_ptr<CTask> CTaskPtr;
class CWorkerThread;
typedef shared_ptr<CWorkerThread> CWorkerPtr;

using namespace std;

class CTask
{
protected:
	double mJDStart;
	double mJDEnd;
	double mJDMean;
	double mWavelengthMean;

	string mFilename;		///< The full filename including path and extension
	string mFilenameShort;	///< The filename less the path and extension.

protected:
	string mExeFolder;
	CWorkerThread * mWorkerThread;

	string mDataDescription; ///< A short few-word phrase to describe the data.
	vector<string> mExtensions; ///< A list of valid extensions for this task.

public:
	CTask(CWorkerThread * WorkerThread);
	virtual ~CTask();

	virtual void BootstrapNext(unsigned int maxBootstrapFailures) = 0;

	virtual void Export(string folder_name) = 0;

	virtual void clearData() = 0;

	virtual CDataInfo getDataInfo() = 0;

	virtual void GetChi(double * chis, unsigned int size) = 0;
	virtual string GetDataDescription();
	virtual vector<string> GetExtensions();
	virtual unsigned int GetNData() = 0;
	virtual int GetNDataFiles() = 0;
	virtual void GetUncertainties(double * residuals, unsigned int size) = 0;

	virtual void InitGL() {};
	virtual void InitCL() {};

	virtual CDataInfo OpenData(string filename) = 0;

	virtual void RemoveData(unsigned int data_index) = 0;

	static string StripPath(string filename);
	static string StripExtension(string filename, vector<string> & valid_extensions);

};

#endif /* CTASK_H_ */
