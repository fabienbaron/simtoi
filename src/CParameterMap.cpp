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

#include "CParameterMap.h"
#include "CParameter.h"

CParameterMap::CParameterMap()
{
	mID = "NOT_IMPLEMENTED_BY_DEVELOPER";
	mName = "NOT_IMPLEMENTED_BY_DEVELOPER";
}

CParameterMap::~CParameterMap()
{
	// TODO Auto-generated destructor stub
}

/// Adds an additional parameter for this model with no help text
unsigned int CParameterMap::addParameter(string internal_name, double value, double min, double max, bool free, double step_size,
		string human_name, unsigned int decimal_places)
{
	return addParameter(internal_name, value, min, max, free, step_size, human_name, string(), decimal_places);
}

/// Adds an additional parameter for this model
unsigned int CParameterMap::addParameter(string internal_name, double value, double min, double max, bool free, double step_size,
		string human_name, string help, unsigned int decimal_places)
{
	// create the parameter, set some default values.
	CParameter temp;
	// Disable bounds checking during setup.
	temp.toggleBoundsChecks(false);
	// Set the values
	temp.setID(internal_name);
	temp.setMin(min);
	temp.setMax(max);
	temp.setValue(value);
	temp.setFree(free);
	temp.setStepSize(step_size);
	temp.setHumanName(human_name);
	temp.setHelpText(help);
	temp.setDecimalPlaces(decimal_places);
	// Enable bounds checking.
	temp.toggleBoundsChecks(true);

	// append it to the vector
	mParams[internal_name] = temp;

	// return the parameter number
	return mParams.size() - 1;
}

/// Clears all flags set on the parameters
void CParameterMap::clearFlags()
{
	for(map<string,CParameter>::iterator it = mParams.begin(); it != mParams.end(); ++it)
		it->second.clearFlags();
}

/// Returns up to n_params values of the parameters for this object.
unsigned int CParameterMap::getAllParameters(double * params, unsigned int n_params, bool normalize_value)
{
	unsigned int n = 0;
	for(auto it: mParams)
	{
		if(n > n_params)
			break;

		params[n] = it.second.getValue(normalize_value);
		n++;
	}

	return n;
}

/// Copies the nominal values of the free parameters into the `params` array
///
/// @param params A double array of size 'n_params' to which the parameters will be copied
/// @param n_params The size of `params`
/// @param normalize_value Whether or not the parameters should be normalized.
unsigned int CParameterMap::getFreeParameters(double * params, unsigned int n_params, bool normalize_value)
{
	unsigned int n = 0;
	for(auto it: mParams)
	{
		if(n > n_params)
			break;

		if(it.second.isFree())
		{
			params[n] = it.second.getValue(normalize_value);
			n++;
		}
	}

	return n;
}

/// Returns a vector full of pairs of min (first) max (second) values for
/// the free parameters.
vector<pair<double,double> > CParameterMap::getFreeParameterMinMaxes()
{
	vector< pair<double, double> > min_maxes;

	for(auto it: mParams)
	{
		if(it.second.isFree())
		{
			pair<double, double> tmp;
			tmp.first = it.second.getMin();
			tmp.second = it.second.getMax();
			min_maxes.push_back(tmp);
		}
	}

	return min_maxes;
}

/// Returns the step sizes of the free parameters.
unsigned int CParameterMap::getFreeParameterStepSizes(double * steps, unsigned int size)
{
	int n = 0;

	for(auto it: mParams)
	{
		if(n > size)
			break;

		if(it.second.isFree())
		{
			steps[n] = it.second.getStepSize();
			n++;
		}
	}

	return n;
}

/// Counts the number of free parameters in the map.
unsigned int CParameterMap::getFreeParameterCount()
{
	unsigned int n = 0;
	for(auto it: mParams)
		if(it.second.isFree()) n++;

	return n;
}

/// \brief Returns a vector of strings containing the names of the free parameters
/// prefixed with the name of the parent object.
vector<string> CParameterMap::getFreeParameterNames()
{
	vector<string> tmp;
	for(auto it: mParams)
	{
		if(it.second.isFree())
			tmp.push_back(mName + '.' + it.second.getHumanName());
	}

	return tmp;
}

/// Returns a reference to the specified parameter or throws an out_of_range
/// exception if the key does not exist.
CParameter & CParameterMap::getParameter(string id)
{
	return mParams.at(id);
}

/// Determines whether or not ANY of the dirty flags are set for this object
/// If at least one dirty flag is set the object is considered dirty.
bool CParameterMap::isDirty()
{
	bool is_dirty = false;

	for(auto it: mParams)
		is_dirty |= it.second.isDirty();

	return is_dirty;
}

/// \brief Restores parameters values from the JSON value
///
/// Restores parameter values, names, and min/max values from a JSON save file.
/// Parameters are imported by the IDs specified in the mParams map. If the
/// parameter is not found, the default values provided in the constructor
/// are left intact.
void CParameterMap::restore(Json::Value input)
{
	// Only restore parameters that we expect will exist.
	for(map<string,CParameter>::iterator it = mParams.begin(); it != mParams.end(); ++it)
	{
		CParameter & param = it->second;
		string id = param.getID();

		// If this parameter is not in the JSON file, skip it.
		if(!input.isMember(id))
			continue;

		// Disable bounds checking while restoring values.
		// Hopefully the save files are consistent!
		param.toggleBoundsChecks(false);

		param.setValue(    double( input[id][0u].asDouble() ));
		param.setMin(      double( input[id][1u].asDouble() ));
		param.setMax(      double( input[id][2u].asDouble() ));
		param.setFree(     bool  ( input[id][3u].asBool()   ));
		param.setStepSize( double( input[id][4u].asDouble() ));

		// attempt to restore the decimal places, this is an optional field.
		try{
			param.setDecimalPlaces( int (input[id][5u].asInt()  ));
		}
		catch(...) { }

		// Enable bounds checking after values are restored.
		param.toggleBoundsChecks(true);
	}
}

/// \brief Serializes the parameters to a Json::Value object
///
/// Converts the map into a Json::value object in the following format:
/// 	`name : [value, min, max, free, step_size]`.
///
/// NOTE: This method can be overridden; however, it is suggested that this function
/// be called by any overriding classes to ensure proper object serialization.
/// For example, see `CModel::Serialize`.
Json::Value CParameterMap::serialize()
{
	Json::Value output;

	for(auto it: mParams)
	{
		CParameter & param = it.second;
		Json::Value tmp;

		tmp.append(Json::Value(param.getValue()));
		tmp.append(Json::Value(param.getMin()));
		tmp.append(Json::Value(param.getMax()));
		tmp.append(Json::Value(param.isFree()));
		tmp.append(Json::Value(param.getStepSize()));
		tmp.append(Json::Value(param.getDecimalPlaces()));
		output[param.getID()] = tmp;
	}

	return output;
}

/// Sets the free parameter values from an input array of doubles. Returns the
/// number of values set during the call.
///
/// @param values An input array of parameter values
/// @param n_values The number of elements in values
/// @param normalized_values Whether or not the values are normalized
unsigned int CParameterMap::setFreeParameterValues(double * values, unsigned int n_values, bool normalized_values)
{
	int n = 0;
	for(auto & it: mParams)
	{
		if(n > n_values)
			break;

		if(it.second.isFree())
		{
			it.second.setValue(values[n], normalized_values);
			n++;
		}
	}

	return n;
}

/// \brief Sets the value of the specified parameter to the indicated value
///        while performing bounds/error checking
///
void CParameterMap::setParameter(const string & name, double value, bool is_normalized)
{
	auto parameter = getParameter(name);
	parameter.setValue(value, is_normalized);
}
