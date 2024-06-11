#ifndef K3DSDK_ITRANSFORM_ARRAY_1D_H
#define K3DSDK_ITRANSFORM_ARRAY_1D_H

// K-3D
// Copyright (c) 1995-2005, Timothy M. Shead
//
// Contact: tshead@k-3d.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/** \file
		\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/algebra.h>
#include <k3dsdk/iunknown.h>

namespace k3d
{

/// Abstract interface for a node that defines a 1-dimensional array of transformation matrices
class itransform_array_1d :
	public virtual iunknown
{
public:
	virtual const matrix4 get_element(unsigned long Index1, unsigned long Count1) = 0;

protected:
	itransform_array_1d() {}
	itransform_array_1d(const itransform_array_1d&) {}
	itransform_array_1d& operator = (const itransform_array_1d&) { return *this; }
	virtual ~itransform_array_1d() {}
};

} // namespace k3d

#endif // !K3DSDK_ITRANSFORM_ARRAY_1D_H

