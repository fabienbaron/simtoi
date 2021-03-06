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
 *  Copyright (c) 2012 Brian Kloppenborg
 */

#ifndef CMULTINEST_H_
#define CMULTINEST_H_

#include "CMinimizerThread.h"

class CMultiNest: public CMinimizerThread
{

public:
	CMultiNest();
	virtual ~CMultiNest();

	static shared_ptr<CMinimizerThread> Create();

	static double ComputeLogLikelihood(valarray<double> & residuals, const valarray<double> & uncertainties);

	static void dumper(int &nSamples, int &nlive, int &nPar, double **physLive, double **posterior,
			double **paramConstr, double &maxLogLike, double &logZ, double &INSlogZ, double &logZerr,
			void *context);

	static void log_likelihood(double * Cube, int & ndim, int & npars, double & lnew, void * misc);

	void ResultFromSummaryFile(string multinest_output_dir);
	void run();

};

#endif /* CMULTINEST_H_ */
