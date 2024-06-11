// K-3D
// Copyright (c) 1995-2006, Timothy M. Shead
//
// Contact: tshead@k-3d.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/** \file
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/line3.h>
#include <k3dsdk/vectors.h>

namespace k3d
{

/////////////////////////////////////////////////////////////////////////////
// line3

line3::line3(const vector3& Direction, const point3& Point) :
	direction(Direction),
	point(Point)
{
}

line3::line3(const point3& Point1, const point3& Point2) :
	direction(Point2 - Point1),
	point(Point1)
{
}

} // namespace k3d

