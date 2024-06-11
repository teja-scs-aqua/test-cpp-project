#ifndef K3DSDK_LINEAR_CURVE_H
#define K3DSDK_LINEAR_CURVE_H

// K-3D
// Copyright (c) 1995-2009, Timothy M. Shead
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

namespace linear_curve
{

/// Gathers the member arrays of a linear_curve primitive into a convenient package
class const_primitive
{
public:
	const_primitive(
		const mesh::bools_t& Periodic,
		const mesh::materials_t& Material,
		const mesh::indices_t& CurveFirstPoints,
		const mesh::counts_t& CurvePointCounts,
		const mesh::selection_t& CurveSelections,
		const mesh::indices_t& CurvePoints,
		const mesh::table_t& ConstantAttributes,
		const mesh::table_t& CurveAttributes,
		const mesh::table_t& ParameterAttributes,
		const mesh::table_t& VertexAttributes
		);

	const mesh::bools_t& periodic;
	const mesh::materials_t& material;
	const mesh::indices_t& curve_first_points;
	const mesh::counts_t& curve_point_counts;
	const mesh::selection_t& curve_selections;
	const mesh::indices_t& curve_points;
	const mesh::table_t& constant_attributes;
	const mesh::table_t& curve_attributes;
	const mesh::table_t& parameter_attributes;
	const mesh::table_t& vertex_attributes;
};

/// Gathers the member arrays of a linear_curve primitive into a convenient package
class primitive
{
public:
	primitive(
		mesh::bools_t& Periodic,
		mesh::materials_t& Material,
		mesh::indices_t& CurveFirstPoints,
		mesh::counts_t& CurvePointCounts,
		mesh::selection_t& CurveSelections,
		mesh::indices_t& CurvePoints,
		mesh::table_t& ConstantAttributes,
		mesh::table_t& CurveAttributes,
		mesh::table_t& ParameterAttributes,
		mesh::table_t& VertexAttributes
		);

	mesh::bools_t& periodic;
	mesh::materials_t& material;
	mesh::indices_t& curve_first_points;
	mesh::counts_t& curve_point_counts;
	mesh::selection_t& curve_selections;
	mesh::indices_t& curve_points;
	mesh::table_t& constant_attributes;
	mesh::table_t& curve_attributes;
	mesh::table_t& parameter_attributes;
	mesh::table_t& vertex_attributes;
};

/// Creates a new linear_curve mesh primitive, returning references to its member arrays.
/// The caller is responsible for the lifetime of the returned object.
primitive* create(mesh& Mesh);

/// Tests the given mesh primitive to see if it is a valid linear_curve primitive, returning references to its member arrays, or NULL.
/// The caller is responsible for the lifetime of the returned object.
const_primitive* validate(const mesh& Mesh, const mesh::primitive& GenericPrimitive);
/// Tests the given mesh primitive to see if it is a valid linear_curve primitive, returning references to its member arrays, or NULL.
/// The caller is responsible for the lifetime of the returned object.
primitive* validate(const mesh& Mesh, mesh::primitive& GenericPrimitive);
/// Tests the given mesh primitive to see if it is a valid linear_curve primitive, returning references to its member arrays, or NULL.
/// The caller is responsible for the lifetime of the returned object.
primitive* validate(const mesh& Mesh, pipeline_data<mesh::primitive>& GenericPrimitive);

} // namespace linear_curve

} // namespace k3d

#endif // !K3DSDK_LINEAR_CURVE_H

