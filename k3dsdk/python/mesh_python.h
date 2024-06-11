#ifndef K3DSDK_PYTHON_MESH_PYTHON_H
#define K3DSDK_PYTHON_MESH_PYTHON_H

// K-3D
// Copyright (c) 1995-2007, Timothy M. Shead
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
	\author Timothy M. Shead (tshead@k-3d.com)
*/

#include <k3dsdk/python/instance_wrapper_python.h>

#include <k3dsdk/mesh.h>
#include <boost/python/object.hpp>

namespace k3d
{

namespace python
{

typedef instance_wrapper<k3d::mesh> mesh_wrapper;
typedef instance_wrapper<const k3d::mesh> const_mesh_wrapper;
typedef instance_wrapper<k3d::mesh::primitive> mesh_primitive_wrapper;
typedef instance_wrapper<const k3d::mesh::primitive> const_mesh_primitive_wrapper;

void define_class_mesh();
void define_class_const_mesh();

} // namespace python

} // namespace k3d

#endif // !K3DSDK_PYTHON_MESH_PYTHON_H

