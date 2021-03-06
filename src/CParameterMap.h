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
 * Copyright (c) 2014 Brian Kloppenborg
 */

#ifndef CPARAMETERMAP_H_
#define CPARAMETERMAP_H_

#include <string>
#include <map>
using namespace std;

#include "json/json.h"

#include "CParameter.h"

class CParameterMap
{
protected:
	map<string, CParameter> mParams; ///< The parameters used in this model.
	string mName;					///< A human-readable name for the object. Try to limit to < 60 characters
	string mID;						///< An internal ID for this object

public:
	CParameterMap();
	virtual ~CParameterMap();

	unsigned int addParameter(string internal_name, double value,
			double min, double max, bool free, double step_size,
			string human_name, unsigned int decimal_places=1);
	unsigned int addParameter(string internal_name, double value,
			double min, double max, bool free, double step_size,
			string human_name, string help, unsigned int decimal_places=1);

	void clearFlags();

	unsigned int getAllParameters(double * params, unsigned int n_params, bool normalize_value = false);
	unsigned int getFreeParameters(double * params, unsigned int n_params, bool normalize_value = false);
	unsigned int getFreeParameterCount();
	vector<string> getFreeParameterNames();
	vector<pair<double,double> > getFreeParameterMinMaxes();
	unsigned int getFreeParameterStepSizes(double * steps, unsigned int size);

	const map<string, CParameter> & getParameterMap() { return mParams; };
	virtual string ID() const { return mID; };
	virtual string name() const { return mName; };
	CParameter & getParameter(string id);

	virtual bool isDirty();

	void restore(Json::Value input);

	Json::Value serialize();

	virtual unsigned int setFreeParameterValues(double * values, unsigned int n_values, bool normalized_values = false);
	void setParameter(const string & name, double value, bool is_normalized = false);
};

#endif /* CPARAMETERMAP_H_ */
