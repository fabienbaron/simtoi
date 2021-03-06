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
 *  Copyright (c) 2012, 2015 Fabien Baron
 *  Copyright (c) 2012 Brian Kloppenborg
 */

#include "CGridSearch.h"

#include <tuple>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "CWorkerThread.h"
#include "CModelList.h"

using namespace std;

CGridSearch::CGridSearch()
{
	mID = "gridsearch";
	mName = "Grid Search - global";
}

CGridSearch::~CGridSearch() {
	// TODO Auto-generated destructor stub
}

/// \brief Create a new CGridSearch object
CMinimizerPtr CGridSearch::Create()
{
	return CMinimizerPtr(new CGridSearch());
}

/// \brief Export results from this minimizer.
///
/// Overrides the base class method to copy the best-fit parameters into the
/// mParams buffer.
void CGridSearch::ExportResults()
{
	// Copy the best-fit parameters into the mParams buffer
	for(unsigned int i = 0; i < mNParams; i++)
		mParams[i] = mBestFit[i];

	// Call the base-class function:
	CMinimizerThread::ExportResults();
}

/// \brief A recursive gridsearch function
///
/// Recursively calls gridsearch until the final level is found. At this level
/// the data are simulated and written to mOutputFile. Data are flushed to disk
/// after a level %2 == 0 iteration completes.
void CGridSearch::GridSearch(unsigned int level)
{
	// If we just set the last parameter, get the chi2r
	if(level == mNParams)
	{
	  // Get the chi2r for the data set
	  CModelListPtr model_list = mWorkerThread->GetModelList();
	  model_list->SetFreeParameters(mParams, mNParams, false);
	  mWorkerThread->GetChi(&mChis[0], mChis.size());
	  double chi2r = ComputeChi2r(mChis, mNParams);

	  // Save to the file
	  //	  WriteRow(mParams, mNParams, chi2r, mOutputFile);

	  // If this set of parameters fits better, replace the best-fit params.
	  if(chi2r < mBestFit[mNParams])
	    {
	      for(int i = 0; i < mNParams; i++)
				mBestFit[i] = mParams[i];

	        mBestFit[mNParams] = chi2r;

					printf("\tNew best CHI2r: %lf Params ", chi2r);
					for(int i=0; i < mNParams; i++)
					{
					printf("#%d: %f \t", i, mBestFit[i]);
					}
					printf("\n");
	    }
	}
	else	// Otherwise set the current parameter value and then recursively call the next level.
	{
	  double step = mSteps[level];
	  double min = mMinMax[level].first;
	  double max = mMinMax[level].second;
	  for(double value = min; value < max; value += step)
	    {
	      if(!mRun)
				break;

	      if(level ==0)
				{
					printf("Top level steps: %d/%d -- Top level param %lf\n", (int)rint(fabs((value-min)/step)), (int)rint(fabs((max-min)/step))-1, value);
				}
	      mParams[level] = value;
	      GridSearch(level + 1);
	    }

	  // If we are on an even level, flush the results to a file.
	  if(level % 2 == 0)
	    mOutputFile.flush();
	}
}

/// \brief Initialize the gridsearch minimizer.
void CGridSearch::Init(shared_ptr<CWorkerThread> worker_thread)
{
	CMinimizerThread::Init(worker_thread);

	mSteps.resize(mNParams);
}

/// \brief Run the gridsearch minimzer
void CGridSearch::run()
{
	// Get the min/max ranges for the parameters:
        CModelListPtr model_list = mWorkerThread->GetModelList();
        model_list->GetFreeParameters(mParams, mNParams, true);
	mMinMax = model_list->GetFreeParamMinMaxes();
	model_list->GetFreeParameterSteps(&mSteps[0], mSteps.size());
	vector<string> names = model_list->GetFreeParamNames();

	// Verify that all of the steps are > 0
	for(double step: mSteps)
	{
		if(step <= 0)
			throw runtime_error("Step size for one parameter is zero. Please fix and try again!");
	}

	// Resize the best-fit parameter array to hold the best-fit parameters
	// and their chi2r (as the last element)
	mBestFit.resize(mNParams + 1);
	mBestFit[mNParams] = std::numeric_limits<double>::infinity();	// Init the best-fit chi2r to some bogus value.

	// Open the statistics file for writing:
	stringstream filename;
	filename.str("");
	filename << mSaveDirectory << "/gridsearch.txt";
	mOutputFile.open(filename.str().c_str());
	mOutputFile.width(15);
	mOutputFile.precision(8);
	// write a somewhat descriptive header
	mOutputFile << "# Param0, Param1, ..., ParamN, chi2r" << endl;

	// run the minimizer
	mIsRunning = true;
	cout << "\nGrid search minimization started\n";
	GridSearch(0);
	mIsRunning = false;
       	mOutputFile.close();

	// Print the actual parameters
	cout << "\nGrid search results\n";
	printf("Lowest chi2 achieved: %f\n", mBestFit[mNParams]);
	printf("Best-fit parameters:\n");
	for(int i=0; i < mNParams; i++)
	{
		printf("  P[%d] = %f (%s)\n", i, mBestFit[i], names[i].c_str());
	}

	// Export the results.
	ExportResults();
}
