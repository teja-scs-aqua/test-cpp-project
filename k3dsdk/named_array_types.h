#ifndef K3DSDK_NAMED_ARRAY_TYPES_H
#define K3DSDK_NAMED_ARRAY_TYPES_H

// K-3D
// Copyright (c) 1995-2008, Timothy M. Shead
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

#include <k3dsdk/algebra.h>
#include <k3dsdk/color.h>
#include <k3dsdk/imaterial.h>
#include <k3dsdk/inode.h>
#include <k3dsdk/normal3.h>
#include <k3dsdk/point2.h>
#include <k3dsdk/point3.h>
#include <k3dsdk/point4.h>
#include <k3dsdk/texture3.h>
#include <k3dsdk/types.h>
#include <k3dsdk/vector2.h>
#include <k3dsdk/vector3.h>

#include <boost/mpl/vector/vector30.hpp>

namespace k3d
{

/// Enumerates all of the data types that can be stored using k3d::named_arrays.
/// If you create a named array that stores a type not in this list, some operations will fail with runtime errors.
typedef boost::mpl::vector22<
	bool_t,
	color,
	double_t,
	imaterial*,
	inode*,
	int16_t,
	int32_t,
	int64_t,
	int8_t,
	matrix4,
	normal3,
	point2,
	point3,
	point4,
	string_t,
	texture3,
	uint16_t,
	uint32_t,
	uint64_t,
	uint8_t,
	vector2,
	vector3
	> named_array_types;

} // namespace k3d

#endif // !K3DSDK_NAMED_ARRAY_TYPES_H

