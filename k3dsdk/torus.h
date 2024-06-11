#ifndef K3DSDK_TORUS_H
#define K3DSDK_TORUS_H

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

#include <k3dsdk/mesh.h>

namespace k3d
{

namespace torus
{

/// Gathers the member arrays of a torus primitive into a convenient package
class const_primitive
{
public:
	const_primitive(
		const mesh::matrices_t& Matrices,
		const mesh::materials_t& Materials,
		const mesh::doubles_t& MajorRadii,
		const mesh::doubles_t& MinorRadii,
		const mesh::doubles_t& PhiMin,
		const mesh::doubles_t& PhiMax,
		const mesh::doubles_t& SweepAngles,
		const mesh::selection_t& Selections,
		const mesh::table_t& ConstantAttributes,
		const mesh::table_t& SurfaceAttributes,
		const mesh::table_t& ParameterAttributes);

	const mesh::matrices_t& matrices;
	const mesh::materials_t& materials;
	const mesh::doubles_t& major_radii;
	const mesh::doubles_t& minor_radii;
	const mesh::doubles_t& phi_min;
	const mesh::doubles_t& phi_max;
	const mesh::doubles_t& sweep_angles;
	const mesh::selection_t& selections;
	const mesh::table_t& constant_attributes;
	const mesh::table_t& surface_attributes;
	const mesh::table_t& parameter_attributes;
};

/// Gathers the member arrays of a torus primitive into a convenient package
class primitive
{
public:
	primitive(
		mesh::matrices_t& Matrices,
		mesh::materials_t& Materials,
		mesh::doubles_t& MajorRadii,
		mesh::doubles_t& MinorRadii,
		mesh::doubles_t& PhiMin,
		mesh::doubles_t& PhiMax,
		mesh::doubles_t& SweepAngles,
		mesh::selection_t& Selections,
		mesh::table_t& ConstantAttributes,
		mesh::table_t& SurfaceAttributes,
		mesh::table_t& ParameterAttributes);

	mesh::matrices_t& matrices;
	mesh::materials_t& materials;
	mesh::doubles_t& major_radii;
	mesh::doubles_t& minor_radii;
	mesh::doubles_t& phi_min;
	mesh::doubles_t& phi_max;
	mesh::doubles_t& sweep_angles;
	mesh::selection_t& selections;
	mesh::table_t& constant_attributes;
	mesh::table_t& surface_attributes;
	mesh::table_t& parameter_attributes;
};

/// Creates a new torus mesh primitive, returning references to its member arrays.
/// The caller is responsible for the lifetime of the returned object.
primitive* create(mesh& Mesh);

/// Tests the given mesh primitive to see if it is a valid torus primitive, returning references to its member arrays, or NULL.
/// The caller is responsible for the lifetime of the returned object.
const_primitive* validate(const mesh& Mesh, const mesh::primitive& GenericPrimitive);
/// Tests the given mesh primitive to see if it is a valid torus primitive, returning references to its member arrays, or NULL.
/// The caller is responsible for the lifetime of the returned object.
primitive* validate(const mesh& Mesh, mesh::primitive& GenericPrimitive);
/// Tests the given mesh primitive to see if it is a valid torus primitive, returning references to its member arrays, or NULL.
/// The caller is responsible for the lifetime of the returned object.
primitive* validate(const mesh& Mesh, pipeline_data<mesh::primitive>& GenericPrimitive);

} // namespace torus

} // namespace k3d

#endif // !K3DSDK_TORUS_H

