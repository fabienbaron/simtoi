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

#include "CPositionXY.h"

CPositionXY::CPositionXY()
{
	mName = "XY";
	mID = "xy";
	mPositionType = STATIC;

	// Init the position variables, set them to be fixed.
	addParameter("N", 0, -1, 1, false, 0.1, "North", "North (mas) (positive is up on the screen)", 2);
	addParameter("E", 0, -1, 1, false, 0.1, "East", "East (mas) (positive is left on the screen)", 2);
}

CPositionXY::~CPositionXY()
{
	// Do nothing
}

CPositionPtr CPositionXY::Create()
{
	return CPositionPtr(new CPositionXY());
}

void CPositionXY::GetXYZ(double & x, double & y, double & z)
{
	// Astronomical convention has (North,East) = (Up,Left),
	// OpenGL has (x,y) = (right,up)
	// So we reassign (x,y) = (-East, North)

	x = -1 * mParams["E"].getValue();
	y = mParams["N"].getValue();

	// By default
	z = 0;
}
