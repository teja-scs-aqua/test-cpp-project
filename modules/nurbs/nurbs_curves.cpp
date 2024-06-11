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

/** \file
	\author Carsten Haubold (CarstenHaubold@web.de)
	\author Bart Janssens (bart.janssens@lid.kviv.be)
*/

#include <k3dsdk/metadata.h>
#include <k3dsdk/metadata_keys.h>
#include <k3dsdk/nurbs_curve.h>
#include <k3dsdk/table_copier.h>

#include "nurbs_curves.h"
#include "utility.h" // for knot_nomalizer

#include <boost/scoped_ptr.hpp>
#include <sstream>

// For linear system solve in the approximate function
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>

namespace ublas = boost::numeric::ublas;

namespace k3d { namespace nurbs_curve { class primitive; } }

namespace module
{

namespace nurbs
{

namespace detail
{

using namespace k3d; // for easy moving to the SDK

struct inf_norm
{
	inf_norm(const k3d::double_t Threshold = 0.000001) : threshold(Threshold) {}

	const bool_t operator()(const point3& p1, const point3& p2)
	{
		if (fabs(p1[0] - p2[0]) < threshold
				&& fabs(p1[1] - p2[1]) < threshold
				&& fabs(p1[2] - p2[2]) < threshold)
			return true;
		return false;
	}

	const k3d::double_t threshold;
};

template<typename ComparatorT>
void lookup_duplicate_points(const mesh::points_t& Points, mesh::indices_t& PointMap, ComparatorT Equal)
{
	const uint_t point_count = Points.size();
	mesh::bools_t duplicate_points(point_count, false);
	PointMap.resize(point_count);
	for(uint_t i = 0; i != point_count; ++i)
	{
		if(duplicate_points[i])
			continue;
		for(uint_t j = i+1; j != point_count; ++j)
		{
			if(Equal(Points[i], Points[j]))
			{
				duplicate_points[j] = true;
				PointMap[j] = i;
			}
		}
		PointMap[i] = i;
	}
}

} // namespace detail

////////////////
// curve_arrays

curve_arrays::curve_arrays(const k3d::uint_t PointCount, const k3d::uint_t Order) :
		points(PointCount, k3d::point4(0,0,0,0)),
		order(Order)
{
}

curve_arrays::curve_arrays(const k3d::mesh& Mesh, const k3d::nurbs_curve::const_primitive& Primitive, const k3d::uint_t Curve, const k3d::bool_t NormalizeKnots)
{
	order = Primitive.curve_orders[Curve];
	const k3d::uint_t point_count = Primitive.curve_point_counts[Curve];
	const k3d::uint_t points_begin = Primitive.curve_first_points[Curve];
	const k3d::uint_t knots_begin = Primitive.curve_first_knots[Curve];
	const k3d::uint_t knots_end = knots_begin + point_count + order;

	points.resize(point_count);
	knots.resize(knots_end - knots_begin);

	point_attributes = Mesh.point_attributes.clone_types();
	k3d::table_copier point_attribute_copier(Mesh.point_attributes, point_attributes);
	curve_attributes = Primitive.curve_attributes.clone_types();
	k3d::table_copier curve_attribute_copier(Primitive.curve_attributes, curve_attributes);
	parameter_attributes = Primitive.parameter_attributes.clone_types();
	k3d::table_copier parameter_attribute_copier(Primitive.parameter_attributes, parameter_attributes);
	vertex_attributes = Primitive.vertex_attributes.clone_types();
	k3d::table_copier vertex_attribute_copier(Primitive.vertex_attributes, vertex_attributes);

	point_attributes.set_row_count(point_count);
	vertex_attributes.set_row_count(point_count);

	curve_attribute_copier.push_back(Curve);
	parameter_attribute_copier.push_back(2*Curve);
	parameter_attribute_copier.push_back(2*Curve+1);

	for(k3d::uint_t point = 0; point != point_count; ++point)
	{
		const k3d::uint_t curve_point_idx = points_begin + point;
		const k3d::point3& p = Mesh.points->at(Primitive.curve_points[curve_point_idx]);
		const k3d::double_t w = Primitive.curve_point_weights[curve_point_idx];
		points[point] = k3d::point4(p[0]*w, p[1]*w, p[2]*w, w);
		// Note: attributes must be weighted as well for uniform treatment
		point_attribute_copier.copy(1, &Primitive.curve_points[curve_point_idx], &w, point);
		vertex_attribute_copier.copy(1, &curve_point_idx, &w, point);
	}

	for(k3d::uint_t knot_idx = knots_begin; knot_idx != knots_end; ++knot_idx)
	{
		const k3d::uint_t i = knot_idx - knots_begin;
		knots[i] = Primitive.curve_knots[knot_idx];
	}
	if(NormalizeKnots)
	{
		const k3d::double_t min = knots.front();
		const k3d::double_t max = knots.back();
		std::transform(knots.begin(), knots.end(), knots.begin(), knot_normalizer(min, max));
	}
	assert_error(validate());
}

const k3d::bool_t curve_arrays::validate() const
{
	return_val_if_fail(order + points.size() == knots.size(), false);

	return_val_if_fail(point_attributes.empty() || points.size() == point_attributes.row_count(), false);
	return_val_if_fail(curve_attributes.empty() || curve_attributes.row_count() == 1, false);
	return_val_if_fail(parameter_attributes.empty() || parameter_attributes.row_count() == 2, false);
	return_val_if_fail(vertex_attributes.empty() || vertex_attributes.row_count() == points.size(), false);

	return true;
}

/// Append attributes to the given mesh
void curve_arrays::add_curve(k3d::mesh& Mesh, k3d::nurbs_curve::primitive& Primitive)
{
	return_if_fail(validate());

	// Initialize point arrays, if needed
	if(!Mesh.points)
	{
		Mesh.points.create();
		Mesh.point_selection.create();
		Mesh.point_attributes = k3d::table();
	}

	// Initialize attributes
	const k3d::uint_t old_point_count = Mesh.points->size();
	const k3d::uint_t old_vertex_count = Primitive.curve_points.size();
	if(Mesh.point_attributes.empty())
	{
		Mesh.point_attributes = point_attributes.clone_types();
		Mesh.point_attributes.set_row_count(old_point_count);
	}
	if(Primitive.vertex_attributes.empty())
	{
		Primitive.vertex_attributes = vertex_attributes.clone_types();
		Primitive.vertex_attributes.set_row_count(old_vertex_count);
	}
	if(Primitive.curve_attributes.empty())
	{
		Primitive.curve_attributes = curve_attributes.clone_types();
		Primitive.curve_attributes.set_row_count(Primitive.curve_first_points.size());
	}
	if(Primitive.parameter_attributes.empty())
	{
		Primitive.parameter_attributes = parameter_attributes.clone_types();
		Primitive.parameter_attributes.set_row_count(2*Primitive.curve_first_points.size());
	}

	// Copy the structure
	k3d::mesh::points_t points3;
	k3d::mesh::weights_t weights;
	const k3d::uint_t points_end = points.size();
	for(k3d::uint_t i = 0; i != points_end; ++i)
	{
		const k3d::point4 p = points[i];
		weights.push_back(p[3]);
		points3.push_back(dehomogenize(p));
	}
	k3d::nurbs_curve::add_curve(Mesh, Primitive, order, points3, weights, knots);
	// Take care of the attributes
	k3d::table_copier point_attribute_copier(point_attributes, Mesh.point_attributes);
	k3d::table_copier vertex_attribute_copier(vertex_attributes, Primitive.vertex_attributes);
	const k3d::uint_t point_count = points.size();
	for(k3d::uint_t point_idx = 0; point_idx != point_count; ++point_idx)
	{
		// "unweight" attributes
		const k3d::point4& point = points[point_idx];
		const k3d::double_t weight = 1/point[3];
		point_attribute_copier.push_back(1, &point_idx, &weight);
		vertex_attribute_copier.push_back(1, &point_idx, &weight);
	}
	k3d::table_copier curve_attribute_copier(curve_attributes, Primitive.curve_attributes);
	k3d::table_copier parameter_attribute_copier(parameter_attributes, Primitive.parameter_attributes);
	curve_attribute_copier.push_back(0);
	parameter_attribute_copier.push_back(0);
	parameter_attribute_copier.push_back(1);

	// Ensure the selection array is correctly sized
	Primitive.curve_selections.resize(Primitive.curve_first_points.size(), 0.0);
}

void curve_arrays::resize(const k3d::uint_t Size, const k3d::uint_t Order)
{
	points.resize(Size);
	knots.resize(Size + Order);
	point_attributes.set_row_count(Size);
	vertex_attributes.set_row_count(Size);
	order = Order;
}

const curve_arrays::curve_value curve_arrays::evaluate(const k3d::double_t U) const
{
	curve_arrays tmp;
	curve_copier tmp_copier(*this, tmp);
	curve_value result;
	const k3d::uint_t point_count = points.size();
	if(U <= knots.front())
	{
		tmp_copier.push_back(0);
	}
	else if(U >= knots.back())
	{
		tmp_copier.push_back(point_count - 1);
	}
	else
	{
		k3d::mesh::knots_t bases;
		basis_functions(bases, knots, order, U);
		const k3d::uint_t knot_idx = std::find_if(knots.begin(), knots.end(), find_first_knot_after(U)) - knots.begin();
		const k3d::uint_t first_point = knot_idx != knots.size() ? knot_idx - order : knot_idx - (2*order);
		std::vector<k3d::uint_t> indices;
		for(k3d::uint_t i = 0; i != order; ++i)
		{
			indices.push_back(first_point+i);
		}
		tmp_copier.push_back(order, &indices[0], &bases[0]);
	}
	result.point_attributes = tmp.point_attributes.clone_types();
	result.vertex_attributes = tmp.vertex_attributes.clone_types();
	k3d::table_copier point_attribute_copier(tmp.point_attributes, result.point_attributes);
	k3d::table_copier vertex_attribute_copier(tmp.vertex_attributes, result.vertex_attributes);
	// "unweight" attributes
	const k3d::uint_t point_idx = 0;
	const k3d::double_t weight = 1/result.weight();
	point_attribute_copier.push_back(1, &point_idx, &weight);
	vertex_attribute_copier.push_back(1, &point_idx, &weight);
	result.weighted_position = tmp.points.back();
	return result;
}

const k3d::vector3 curve_arrays::tangent(const k3d::double_t U, const k3d::double_t DeltaU) const
{
	const k3d::double_t u = U > (1.0 - DeltaU) ? U - DeltaU : U;
  curve_value v1 = evaluate(u);
  curve_value v2 = evaluate(u + DeltaU);
	return k3d::normalize(v2.position() - v1.position());
}

void curve_arrays::points3(k3d::mesh::points_t& Points) const
{
	const k3d::uint_t point_count = points.size();
	for(k3d::uint_t i = 0; i != point_count; ++i)
	{
		const k3d::point4& p = points[i];
		const k3d::double_t w = p[3];
		Points.push_back(k3d::point3(p[0]/w, p[1]/w, p[2]/w));
	}
}

void curve_arrays::weights(k3d::mesh::weights_t& Weights) const
{
	const k3d::uint_t point_count = points.size();
	for(k3d::uint_t i = 0; i != point_count; ++i)
		Weights.push_back(points[i][3]);
}

void curve_arrays::append(const curve_arrays& Other)
{
	return_if_fail(Other.validate());
	if(points.empty())
	{
		order = Other.order;
	}
	else
	{
		return_if_fail(order == Other.order);
	}
	const k3d::uint_t other_point_count = Other.points.size();
	curve_copier copier(Other, *this);
	for(k3d::uint_t i = (knots.empty() ? 0 : 1); i != other_point_count; ++i)
		copier.push_back(i);
	const k3d::uint_t knots_begin = knots.size() ? order : 0;
	const k3d::uint_t knots_end = knots_begin + other_point_count + (knots.size() ? 0 : order);
	k3d::double_t knot_offset = knots.size() ? knots.back() - Other.knots.front() : 0;
	if(!knots.empty())
		knots.pop_back();
	for(k3d::uint_t knot = knots_begin; knot != knots_end; ++knot)
		knots.push_back(Other.knots[knot] + knot_offset);
}

////////////////////////

////////////// curve_copier ////////////
curve_copier::curve_copier(const curve_arrays& Source, curve_arrays& Destination) : m_source(Source), m_destination(Destination)
{
	const k3d::uint_t destination_size = m_destination.points.size();
	if(m_destination.point_attributes.empty())
	{
		m_destination.point_attributes = m_source.point_attributes.clone_types();
		m_destination.point_attributes.set_row_count(destination_size);
	}
	m_point_attribute_copier.reset(new k3d::table_copier(m_source.point_attributes, m_destination.point_attributes));
	if(m_destination.vertex_attributes.empty())
	{
		m_destination.vertex_attributes = m_source.vertex_attributes.clone_types();
		m_destination.vertex_attributes.set_row_count(destination_size);
	}
	m_vertex_attribute_copier.reset(new k3d::table_copier(m_source.vertex_attributes, m_destination.vertex_attributes));
	if(m_destination.curve_attributes.empty())
	{
		m_destination.curve_attributes = m_source.curve_attributes.clone_types();
		if(m_source.curve_attributes.row_count())
		{
			k3d::table_copier curve_attrib_copier(m_source.curve_attributes, m_destination.curve_attributes);
			curve_attrib_copier.push_back(0);
		}
	}
	if(m_destination.parameter_attributes.empty())
	{
		m_destination.parameter_attributes = m_source.parameter_attributes.clone_types();
		if(m_source.parameter_attributes.row_count() == 2)
		{
			k3d::table_copier parameter_attrib_copier(m_source.parameter_attributes, m_destination.parameter_attributes);
			parameter_attrib_copier.push_back(0);
			parameter_attrib_copier.push_back(1);
		}
	}
}

void curve_copier::push_back(const k3d::uint_t Index)
{
	m_destination.points.push_back(m_source.points[Index]);
	m_point_attribute_copier->push_back(Index);
	m_vertex_attribute_copier->push_back(Index);
}

void curve_copier::push_back(const k3d::uint_t Count, const k3d::uint_t* Indices, const k3d::double_t* Weights)
{
	m_destination.points.push_back(k3d::point4(0,0,0,0));
	for(k3d::uint_t i = 0; i != Count; ++i)
	{
		m_destination.points.back() += k3d::to_vector(m_source.points[Indices[i]] * Weights[i]);
	}
	m_point_attribute_copier->push_back(Count, Indices, Weights);
	m_vertex_attribute_copier->push_back(Count, Indices, Weights);
}

void curve_copier::copy(const k3d::uint_t SourceIndex, const k3d::uint_t TargetIndex)
{
	m_destination.points[TargetIndex] = m_source.points[SourceIndex];
	m_point_attribute_copier->copy(SourceIndex, TargetIndex);
	m_vertex_attribute_copier->copy(SourceIndex, TargetIndex);
}

void curve_copier::copy(const k3d::uint_t Count, const k3d::uint_t* Indices, const k3d::double_t* Weights, const k3d::uint_t TargetIndex)
{
	k3d::point4 result(0,0,0,0);
	for(k3d::uint_t i = 0; i != Count; ++i)
	{
		result += k3d::to_vector(m_source.points[Indices[i]] * Weights[i]);
	}
	m_destination.points[TargetIndex] = result;
	m_point_attribute_copier->copy(Count, Indices, Weights, TargetIndex);
	m_vertex_attribute_copier->copy(Count, Indices, Weights, TargetIndex);
}

//////////////////////////////////////

void replace_duplicate_points(k3d::mesh& Mesh, const k3d::mesh::primitives_t::iterator Begin, const k3d::mesh::primitives_t::iterator End)
{
	k3d::mesh clone;
	clone.points = Mesh.points;
	clone.point_selection = Mesh.point_selection;
	clone.point_attributes = Mesh.point_attributes;
	clone.primitives.insert(clone.primitives.end(), Begin, End);
	replace_duplicate_points(clone);
	std::copy(clone.primitives.begin(), clone.primitives.end(), Begin);
}

void replace_duplicate_points(k3d::mesh& Mesh)
{
	// Create a mapping between duplicate points and their replacement
	k3d::mesh::indices_t point_map;
	detail::lookup_duplicate_points(*Mesh.points, point_map, detail::inf_norm());
	k3d::mesh::remap_points(Mesh, point_map);
}

void copy_curve(k3d::mesh& OutputMesh, k3d::nurbs_curve::primitive& OutputCurves, const k3d::mesh& InputMesh, const k3d::nurbs_curve::const_primitive& InputCurves, const k3d::uint_t Curve)
{
	curve_arrays input_arrays(InputMesh, InputCurves, Curve);
	input_arrays.add_curve(OutputMesh, OutputCurves);
	OutputCurves.material = InputCurves.material;
	OutputCurves.curve_selections.back() = InputCurves.curve_selections[Curve];
}

void update_indices(k3d::mesh::indices_t& Indices, const k3d::uint_t Start, const k3d::int32_t Offset)
{
	for(k3d::uint_t i = 0; i != Indices.size(); ++i)
	{
		if(Indices[i] >= Start)
			Indices[i] += Offset;
	}
}

// Remove the item at Index
template<typename ArrayT>
void erase(ArrayT& Array, const k3d::uint_t Index)
{
	return_if_fail(Index < Array.size());
	Array.erase(Array.begin() + Index);
}

void delete_curve(k3d::nurbs_curve::primitive& Curves, const k3d::uint_t Curve)
{
	const k3d::uint_t curve_count = Curves.curve_first_points.size();

	k3d::table curve_attributes = Curves.curve_attributes.clone_types();
	k3d::table_copier uniform_copier(Curves.curve_attributes, curve_attributes);
	for(k3d::uint_t i = 0; i != Curve; ++i)
		uniform_copier.push_back(i);
	for(k3d::uint_t i = Curve + 1; i != curve_count; ++i)
		uniform_copier.push_back(i);
	k3d::table parameter_attributes = Curves.parameter_attributes.clone_types();
	k3d::table_copier parameter_copier(Curves.parameter_attributes, parameter_attributes);
	k3d::uint_t curve_start_index = Curves.curve_first_points[Curve];
	k3d::uint_t curve_end_index = curve_start_index + Curves.curve_point_counts[Curve];
	for(k3d::uint_t i = 0; i != curve_start_index; ++i)
		parameter_copier.push_back(i);
	for(k3d::uint_t i = curve_end_index; i != Curves.curve_points.size(); ++i)
		parameter_copier.push_back(i);

	return_if_fail(Curve < curve_count);
	// Erase point indices and weights
	k3d::mesh::indices_t::iterator points_begin = Curves.curve_points.begin() + Curves.curve_first_points[Curve];
	k3d::mesh::indices_t::iterator points_end = points_begin + Curves.curve_point_counts[Curve];
	Curves.curve_points.erase(points_begin, points_end);
	update_indices(Curves.curve_first_points, Curves.curve_first_points[Curve] + Curves.curve_point_counts[Curve], -Curves.curve_point_counts[Curve]);
	k3d::mesh::weights_t::iterator weights_begin = Curves.curve_point_weights.begin() + Curves.curve_first_points[Curve];
	k3d::mesh::weights_t::iterator weights_end = weights_begin + Curves.curve_point_counts[Curve];
	Curves.curve_point_weights.erase(weights_begin, weights_end);
	// Erase knots
	k3d::mesh::knots_t::iterator knots_begin = Curves.curve_knots.begin() + Curves.curve_first_knots[Curve];
	k3d::mesh::knots_t::iterator knots_end = knots_begin + Curves.curve_point_counts[Curve] + Curves.curve_orders[Curve];
	Curves.curve_knots.erase(knots_begin, knots_end);
	update_indices(Curves.curve_first_knots, Curves.curve_first_knots[Curve] + Curves.curve_point_counts[Curve] + Curves.curve_orders[Curve], -(Curves.curve_point_counts[Curve] + Curves.curve_orders[Curve]));

	erase(Curves.curve_first_knots, Curve);
	erase(Curves.curve_first_points, Curve);
	erase(Curves.curve_orders, Curve);
	erase(Curves.curve_point_counts, Curve);
	erase(Curves.curve_selections, Curve);
}

void normalize_knot_vector(k3d::nurbs_curve::primitive& NurbsCurve, const k3d::uint_t Curve)
{
	k3d::mesh::knots_t::iterator curve_knots_begin = NurbsCurve.curve_knots.begin() + NurbsCurve.curve_first_knots[Curve];
	k3d::mesh::knots_t::iterator curve_knots_end = curve_knots_begin + NurbsCurve.curve_point_counts[Curve] + NurbsCurve.curve_orders[Curve];

	k3d::double_t min = *curve_knots_begin;
	k3d::double_t max = *(curve_knots_end - 1);

	std::transform(curve_knots_begin, curve_knots_end, curve_knots_begin, knot_normalizer(min, max));
}

void normalize_knot_vector(k3d::mesh::knots_t& Knots)
{
	std::transform(Knots.begin(), Knots.end(), Knots.begin(), knot_normalizer(Knots.front(), Knots.back()));
}

void close_curve(k3d::mesh& OutputMesh, k3d::nurbs_curve::primitive& OutputCurves, const k3d::mesh& InputMesh, const k3d::nurbs_curve::const_primitive& InputCurves, const k3d::uint_t Curve, const k3d::bool_t keep_ends)
{
	/*
			- get both end points
			if(keep_ends)
					. add the middle of the points as new one and use the old ends as CV
			else . replace them by their arithmetical middle
	*/

	// Weights and indices for the middle of the endpoints
	const k3d::uint_t point_count = InputCurves.curve_point_counts[Curve];
	const k3d::uint_t indices[] = {0, point_count - 1};
	const k3d::double_t weights[] = {0.5, 0.5};

	if (!keep_ends)
	{
		// New end points become the arithmetic mean of the old points
		curve_arrays curve(InputMesh, InputCurves, Curve);
		curve_copier copier(curve, curve);
		copier.copy(2, indices, weights, 0);
		copier.copy(2, indices, weights, point_count - 1);
		curve.add_curve(OutputMesh, OutputCurves);
	}
	else
	{
		const curve_arrays input_curve(InputMesh, InputCurves, Curve, true);
		curve_arrays output_curve;
		curve_copier input_copier(input_curve, output_curve);
		output_curve.order = input_curve.order;
		const k3d::uint_t point_count = input_curve.points.size();
		//take the first knot different from 0 and the last one different from 1.0, double the
		k3d::uint_t first = 0;
		const k3d::uint_t knots_end = input_curve.knots.size();

    while (first < knots_end && input_curve.knots[first] == 0.0)
		{
			first++;
		}

		k3d::uint_t last = knots_end - 1;

		while (input_curve.knots[last] == 1.0 && last != 0)
		{
			last--;
		}

		output_curve.knots = input_curve.knots;
		//insert knot at the end
		double diff = 1.0 - input_curve.knots[last];

		for (k3d::uint_t i = last + 1; i < knots_end; i++)
			output_curve.knots[i] += diff;

		//insert knot at the beginning
		diff = input_curve.knots[first];

		for (int i = first - 1; i >= 0; i--)
		{
			output_curve.knots[i] -= diff;
		}

		k3d::mesh::knots_t::iterator last_iter = output_curve.knots.begin() + last + 1;
		output_curve.knots.insert(last_iter, 1.0);
		k3d::mesh::knots_t::iterator first_iter = output_curve.knots.begin() + first;
		output_curve.knots.insert(first_iter, 0.0);

		//insert point indices to the new point
		input_copier.push_back(2, indices, weights); // First point becomes the midpoint
		for(k3d::uint_t i = 0; i != point_count; ++i) // Copy the original points
			input_copier.push_back(i);
		input_copier.push_back(2, indices, weights); // Last point is also the aveerage of the old endpoints

		normalize_knot_vector(output_curve.knots);
		output_curve.add_curve(OutputMesh, OutputCurves);
	}
}

const k3d::uint_t factorial(const k3d::uint_t n)
{
	if (n == 0) return 1;
	if (n < 3) return n;
	return factorial(n -1)*n;
}

const k3d::double_t binomial_coefficient(const k3d::uint_t n, const k3d::uint_t k)
{
	return_val_if_fail(n >= k, 0.0);
	k3d::double_t result = factorial(n);
	result /= factorial(k) * factorial(n - k);
	return result;
}

void fill_bezalfs(std::vector<std::vector<double> >& bezalfs, int power, int t)
{
	int ph = power + t;
	int ph2 = ph / 2;

	bezalfs[0][0] = 1.0;
	bezalfs[ph][power] = 1.0;

	for (int i = 1; i <= ph2; i++)
	{
		double inv = 1.0 / binomial_coefficient(ph, i);
		int mpi = std::min(power, i);
		for (int j = std::max(0, i - t); j <= mpi; j++)
		{
			bezalfs[i][j] = inv * binomial_coefficient(power, j) * binomial_coefficient(t, i - j);
		}
	}

	for (int i = ph2 + 1; i <= ph - 1; i++)
	{
		int mpi = std::min(power, i);
		for (int j = std::max(0, i - t); j <= mpi; j++)
		{
			bezalfs[i][j] = bezalfs[ph - i][power - j];
		}
	}
}

/// Helper function to make a homogeneous point out of a point3
const k3d::point4 homogeneous(const k3d::point3& p, const k3d::double_t w)
{
	return k3d::point4(k3d::point4(p[0]*w, p[1]*w, p[2]*w, w));
}

void log_points(const curve_arrays::points4_t& Points)
{
	for(k3d::uint_t i = 0; i != Points.size(); ++i)
	{
		const k3d::point4& p = Points[i];
		k3d::log() << "  " << p[0] << ", " << p[1] << ", " << p[2] << ", " << p[3] << std::endl;
	}
}

void elevate_curve_degree(curve_arrays& Curve, const k3d::uint_t Elevations)
{
	for(k3d::uint_t level = 0; level != Elevations; ++level)
	{
		const int t = 1;
		int power = Curve.order - 1;
		const k3d::uint_t nr_points = Curve.points.size();

		std::vector<std::vector<double> > bezalfs(power + t + 1, std::vector<double>(power + 1, 1.0));
		curve_arrays bpts(power + 1, Curve.order);
		curve_arrays ebpts(power + t + 1, Curve.order);
		curve_arrays next_bpts(power - 1, Curve.order);

		curve_copier bpts_input_copier(Curve, bpts);
		curve_copier bpts_self_copier(bpts, bpts);
		curve_copier next_bpts_bpts_copier(bpts, next_bpts);
		curve_copier bpts_next_copier(next_bpts, bpts);
		curve_copier ebpts_bpts_copier(bpts, ebpts);
		curve_copier ebpts_self_copier(ebpts, ebpts);

		std::vector<double> alphas(power - 1, 0.0);

		curve_arrays new_curve(2*(nr_points + t), Curve.order + 1);
		new_curve.knots.assign(2*(nr_points + Curve.order + t), 0.0);

		curve_copier new_point_input_copier(Curve, new_curve);
		curve_copier new_point_self_copier(new_curve, new_curve);
		curve_copier new_point_ebpts_copier(ebpts, new_curve);

		fill_bezalfs(bezalfs, power, t);

		int m = nr_points + power;
		int mh = power + t;
		int kind = mh + 1;
		int r = -1;
		int a = power;
		int b = Curve.order;
		int cind = 1;
		double ua = Curve.knots.front();

		new_point_input_copier.copy(0, 0);

		for (int i = 0; i <= power + t; i++)
			new_curve.knots.push_back(ua);

		//initialize first bezier segment
		for (int i = 0; i <= power; i++)
		{
			bpts_input_copier.copy(i, i);
		}

		while (b < m)//big loop through knot vector
		{
			int i = b;
			while (b < m && fabs(Curve.knots[b] - Curve.knots[b + 1]) < 0.00001)
				b++;
			int mul = b - i + 1;
			mh = mh + mul + t;
			double ub = Curve.knots[b];
			int oldr = r;
			r = power - mul;
			//insert knot ub r times
			int lbz, rbz;
			if (oldr > 0)
				lbz = (oldr + 2) / 2;
			else
				lbz = 1;

			if (r > 0)
				rbz = power + t - ((r + 1) / 2);
			else
				rbz = power + t;

			if (r > 0)
			{
				//insert knot to get bezier segment
				double numer = ub - ua;
				for (int k = power; k > mul; k--)
				{
					alphas[k - mul - 1] = numer / (Curve.knots[a + k] - ua);
				}
				for (int j = 1; j <= r; j++)
				{
					int save = r - j;
					int s = mul + j;
					for (int k = power; k >= s; k--)
					{
						k3d::double_t weights[] = {alphas[k - s], (1.0 - alphas[k - s])};
						k3d::uint_t indices[] = {static_cast<k3d::uint_t>(k), static_cast<k3d::uint_t>(k-1)};
						bpts_self_copier.copy(2, indices, weights, k);
					}
					next_bpts_bpts_copier.copy(power, save);
				}
			}//end of "insert knot"
			//degree elevate bezier
			for (int i = lbz; i <= power + t; i++)
			{
				//only points lbz, ... , power+t are used here
				int mpi = std::min(power, i);
				std::vector<k3d::uint_t> indices;
				std::vector<k3d::double_t> weights;
				for (int j = std::max(0, i - t); j <= mpi; j++)
				{
					indices.push_back(j);
					weights.push_back(bezalfs[i][j]);
				}
				ebpts_bpts_copier.copy(indices.size(), &indices[0], &weights[0], i);
			}

			if (oldr > 1)
			{
				//must remove knot ua oldr times
				int first = kind - 2;
				int last = kind;
				double den = ub - ua;
				double bet = (ub - new_curve.knots[kind - 1]) / den;
				//knot removal loop
				for (int tr = 1; tr < oldr; tr++)
				{
					int i = first;
					int j = last;
					int kj = j - kind + 1;
					while (j - i > tr)//loop and compute the new control points for one removal step
					{
						if (i < cind)
						{
							double alf = (ub - new_curve.knots[i]) / (ua - new_curve.knots[i]);
							k3d::uint_t indices[] = {static_cast<k3d::uint_t>(i), static_cast<k3d::uint_t>(i-1)};
							k3d::double_t weights[] = {alf, (1.0 - alf)};
							new_point_self_copier.copy(2, indices, weights, i);
						}
						if (j >= lbz)
						{
							if (j - tr <= kind - power - t + oldr)
							{
								double gam = (ub - new_curve.knots[j - tr]) / den;
								k3d::uint_t indices[] = {static_cast<k3d::uint_t>(kj), static_cast<k3d::uint_t>(kj+1)};
								k3d::double_t weights[] = {gam, 1.0-gam};
								ebpts_self_copier.copy(2, indices, weights, kj);
							}
							else
							{
								k3d::uint_t indices[] = {static_cast<k3d::uint_t>(kj), static_cast<k3d::uint_t>(kj+1)};
								k3d::double_t weights[] = {bet, 1.0-bet};
								ebpts_self_copier.copy(2, indices, weights, kj);
							}
						}
						i++;
						j--;
						kj--;
					}
					first--;
					last++;
				}
			}//end of removing knot ua
			if (a != power)
			{
				for (int i = 0; i < power + t - oldr; i++)
				{
					new_curve.knots[kind] = ua;
					kind++;
				}
			}
			for (int j = lbz; j <= rbz; j++)
			{
				new_point_ebpts_copier.copy(j, cind);
				cind++;
			}

			if (b < m)
			{
				for (int j = 0; j < r; j++)
				{
					bpts_next_copier.copy(j, j);
				}
				for (int j = r; j <= power; j++)
				{
					bpts_input_copier.copy(b - power + j, j);
				}
				a = b;
				b++;
				ua = ub;
			}
			else//end knot
			{
				for (int i = 0; i <= power + t; i++)
				{
					new_curve.knots[kind + i] = ub;
				}
			}
		}//end while loop (b < m)

		const k3d::uint_t new_n = mh - power - t;
		new_point_input_copier.copy(Curve.points.size()-1, new_n - 1);
		new_curve.resize(new_n, Curve.order + t);
		Curve = new_curve;
	}
}

void elevate_curve_degree(k3d::mesh& OutputMesh, k3d::nurbs_curve::primitive& OutputCurves, const k3d::mesh& InputMesh, const k3d::nurbs_curve::const_primitive& InputCurves, const k3d::uint_t Curve, const k3d::uint_t Elevations)
{
	curve_arrays input_curve(InputMesh, InputCurves, Curve);
	elevate_curve_degree(input_curve, Elevations);
	input_curve.add_curve(OutputMesh, OutputCurves);
}

void flip_curve(k3d::nurbs_curve::primitive& NurbsCurves, const k3d::uint_t curve)
{
	const k3d::uint_t curve_point_begin = NurbsCurves.curve_first_points[curve];
	const k3d::uint_t curve_point_end = curve_point_begin + NurbsCurves.curve_point_counts[curve];

	k3d::uint_t curve_knots_begin = NurbsCurves.curve_first_knots[curve];
	k3d::uint_t curve_knots_end = curve_knots_begin + NurbsCurves.curve_point_counts[curve] + NurbsCurves.curve_orders[curve];
	k3d::mesh::knots_t::iterator curve_knots_begin_it = NurbsCurves.curve_knots.begin() + curve_knots_begin;
	k3d::mesh::knots_t::iterator curve_knots_end_it = NurbsCurves.curve_knots.begin() + curve_knots_end;
	const k3d::double_t max_knot = NurbsCurves.curve_knots[curve_knots_end-1];
	for(k3d::uint_t i = curve_knots_begin; i != curve_knots_end; ++i) NurbsCurves.curve_knots[i] = max_knot - NurbsCurves.curve_knots[i];
	std::reverse(curve_knots_begin_it, curve_knots_end_it);

	//flip point indices
	k3d::mesh::indices_t::iterator curve_points_begin_iter = NurbsCurves.curve_points.begin() + curve_point_begin;
	k3d::mesh::indices_t::iterator curve_points_end_iter = NurbsCurves.curve_points.begin() + curve_point_end;
	std::reverse(curve_points_begin_iter, curve_points_end_iter);

	//flip weights
	k3d::mesh::weights_t::iterator point_weights_begin_iter = NurbsCurves.curve_point_weights.begin() + curve_point_begin;
	k3d::mesh::weights_t::iterator point_weights_end_iter = NurbsCurves.curve_point_weights.begin() + curve_point_end;
	std::reverse(point_weights_begin_iter, point_weights_end_iter);
}

void flip_curve(curve_arrays& Curve)
{
	curve_arrays result;
	result.order = Curve.order;
	result.knots = Curve.knots;

	const k3d::double_t max_knot = result.knots.back();
	for(k3d::uint_t i = 0; i != result.knots.size(); ++i) result.knots[i] = max_knot - result.knots[i];
	std::reverse(result.knots.begin(), result.knots.end());

	curve_copier copier(Curve, result);
	const k3d::uint_t point_count = Curve.points.size();
	for(k3d::uint_t i = 0; i != point_count; ++i)
		copier.push_back(point_count - 1 - i);

	Curve = result;
}

void connect_curves(k3d::mesh& OutputMesh, k3d::nurbs_curve::primitive& OutputCurves, const k3d::mesh& InputMesh, const k3d::uint_t Primitive1Index, const k3d::uint_t Curve1Index, const k3d::bool_t Curve1FirstPointSelection, const k3d::uint_t Primitive2Index, const k3d::uint_t Curve2Index, const k3d::bool_t Curve2FirstPointSelection)
{
	boost::scoped_ptr<k3d::nurbs_curve::const_primitive> curves_prim1(k3d::nurbs_curve::validate(InputMesh, *InputMesh.primitives[Primitive1Index]));
	boost::scoped_ptr<k3d::nurbs_curve::const_primitive> curves_prim2(k3d::nurbs_curve::validate(InputMesh, *InputMesh.primitives[Primitive2Index]));

	curve_arrays curve1(InputMesh, *curves_prim1, Curve1Index);
	curve_arrays curve2(InputMesh, *curves_prim2, Curve2Index);

	if(curve1.order < curve2.order)
	{
		elevate_curve_degree(curve1, curve2.order - curve1.order);
	}
	else
	{
		elevate_curve_degree(curve2, curve1.order - curve2.order);
	}

	if(Curve1FirstPointSelection)
		flip_curve(curve1);
	if(!Curve2FirstPointSelection)
		flip_curve(curve2);

	normalize_knot_vector(curve1.knots);
	normalize_knot_vector(curve2.knots);

	curve_arrays new_curve;
	curve_copier copier1(curve1, new_curve);
	curve_copier copier2(curve2, new_curve);
	new_curve.order = curve1.order; // both orders are equal now

	for(k3d::uint_t point_idx = 0; point_idx != curve1.points.size(); ++point_idx)
	{
		copier1.push_back(point_idx);
	}
	for(k3d::uint_t point_idx = 0; point_idx != curve2.points.size(); ++point_idx)
	{
		copier2.push_back(point_idx);
	}

	for(k3d::uint_t i = 0; i != curve1.points.size(); ++i)
	{
		new_curve.knots.push_back(curve1.knots[i]);
	}
	const k3d::uint_t knots2_begin = new_curve.order;
	const k3d::double_t step = (1 + curve2.knots[knots2_begin] - new_curve.knots.back()) / (new_curve.order + 1);
	for(k3d::uint_t i = 0; i != new_curve.order; ++i)
		new_curve.knots.push_back(new_curve.knots.back() + step);
	for(k3d::uint_t i = 0; i != curve2.points.size(); ++i)
		new_curve.knots.push_back(1 + curve2.knots[knots2_begin + i]);

	new_curve.add_curve(OutputMesh, OutputCurves);
}

void merge_connected_curves(k3d::mesh& OutputMesh, k3d::nurbs_curve::primitive& OutputCurves, const k3d::mesh& InputMesh, const k3d::uint_t InputCurvesPrimIdx)
{
	// make sure all curves are at the same order, and store them in a temporary mesh
	k3d::mesh temp_mesh;
	temp_mesh.points.create();
	temp_mesh.point_selection.create();
	boost::scoped_ptr<k3d::nurbs_curve::primitive> temp_curves(k3d::nurbs_curve::create(temp_mesh));
	k3d::uint_t max_order = 0;
	boost::scoped_ptr<k3d::nurbs_curve::const_primitive> input_curves(k3d::nurbs_curve::validate(InputMesh, *InputMesh.primitives[InputCurvesPrimIdx]));
	const k3d::uint_t curve_count = input_curves->curve_first_points.size();
	temp_curves->material.push_back(input_curves->material.back());
	for(k3d::uint_t curve = 0; curve != curve_count; ++curve)
	{
		const k3d::uint_t order = input_curves->curve_orders[curve];
		max_order = order > max_order ? order : max_order;
	}
	for(k3d::uint_t curve = 0; curve != curve_count; ++curve)
	{
		const k3d::uint_t order = input_curves->curve_orders[curve];
		elevate_curve_degree(temp_mesh, *temp_curves, InputMesh, *input_curves, curve, max_order - order);
	}
	temp_curves->material = input_curves->material;
	replace_duplicate_points(temp_mesh);

	// Get the first and last point of every curve
	k3d::mesh::indices_t first_points, last_points;
	for(k3d::uint_t curve = 0; curve != curve_count; ++curve)
	{
		const k3d::uint_t first_idx = temp_curves->curve_first_points[curve];
		const k3d::uint_t last_idx = first_idx + temp_curves->curve_point_counts[curve] - 1;
		first_points.push_back(temp_curves->curve_points[first_idx]);
		last_points.push_back(temp_curves->curve_points[last_idx]);
	}

	// Find out how many curves share first and last points
	const k3d::uint_t point_count = temp_mesh.points->size();
	k3d::mesh::counts_t first_valences(point_count);
	k3d::mesh::counts_t last_valences(point_count);
	k3d::mesh::bools_t connection_points(point_count);
	for(k3d::uint_t point = 0; point != point_count; ++point)
	{
		first_valences[point] = std::count(first_points.begin(), first_points.end(), point);
		last_valences[point] = std::count(last_points.begin(), last_points.end(), point);
		connection_points[point] = (first_valences[point] + last_valences[point]) == 2;
	}

	// Flip curves in case two curves start or end at the same point
	k3d::mesh::bools_t flipped_curves(curve_count, 0);
	k3d::mesh::indices_t first_point_curves(point_count, 0); // Maps a first point index to its curve
	k3d::mesh::indices_t last_point_curves(point_count, 0); // Maps a last point index to its curve
	k3d::bool_t need_fix = true;
	while(need_fix)
	{
		for(k3d::uint_t curve = 0; curve != curve_count; ++curve)
		{
			const k3d::uint_t first_point = first_points[curve];
			const k3d::uint_t last_point = last_points[curve];
			if(!flipped_curves[curve] && connection_points[first_point] && first_valences[first_point] == 2)
			{
				flip_curve(*temp_curves, curve);
				flipped_curves[curve] = true;
				first_points[curve] = last_points[curve];
				last_points[curve] = first_point;
				first_valences[first_point]--;
				first_valences[last_point]++;
				last_valences[last_point]--;
				last_valences[first_point]++;
			}
			first_point_curves[first_points[curve]] = curve;
			last_point_curves[last_points[curve]] = curve;
		}
		need_fix = false;
		for(k3d::uint_t i = 0; i != first_valences.size(); ++i)
		{
			if(first_valences[i] == 2 && connection_points[i])
				need_fix = true;
		}
	}

	// Follow connected curves and join them into single curves
	k3d::mesh::bools_t added_curves(curve_count, false);
	for(k3d::uint_t checked_curve = 0; checked_curve != curve_count; ++checked_curve)
	{
		if(added_curves[checked_curve])
			continue;
		curve_arrays merged_curve;

		// Find the first curve in this sequence
		k3d::uint_t first_curve;
		for(first_curve = checked_curve; ;)
		{
			if(!connection_points[first_points[first_curve]])
				break;

			first_curve = last_point_curves[first_points[first_curve]];
			if(first_curve == checked_curve)
				break;
		}

		boost::scoped_ptr<k3d::nurbs_curve::const_primitive> const_temp_curves(k3d::nurbs_curve::validate(temp_mesh, *temp_mesh.primitives.back()));
		// Now append all curves in the sequence to the merged curve
		for(k3d::uint_t curve = first_curve; ;)
		{
			curve_arrays tmp_curve(temp_mesh, *const_temp_curves, curve);
			merged_curve.append(tmp_curve);
			added_curves[curve] = true;

			if(!connection_points[last_points[curve]])
				break;

			curve = first_point_curves[last_points[curve]];
			if(curve == first_curve)
				break;
		}
		merged_curve.add_curve(OutputMesh, OutputCurves);
		OutputCurves.curve_selections.back() = 1.0;
	}
	if(OutputCurves.material.empty())
		OutputCurves.material.push_back(input_curves->material.back());

}

/// Predicate to find knot multiplicity
struct find_knot_multiplicity
{
	find_knot_multiplicity(const k3d::double_t KnotValue) : knot_value(KnotValue) {}
	k3d::bool_t operator()(const k3d::double_t TestKnot)
	{
		return std::abs(TestKnot - knot_value) < 0.000001;
	}
	const k3d::double_t knot_value;
};

const k3d::uint_t multiplicity(const k3d::mesh::knots_t& Knots, const k3d::double_t u, const k3d::uint_t Begin, const k3d::uint_t Count)
{
	k3d::mesh::knots_t::const_iterator begin = Knots.begin() + Begin;
	k3d::mesh::knots_t::const_iterator end = begin + Count;
	return std::count_if(begin, end, find_knot_multiplicity(u));
}

/// Helper function to enable consecutive knot insertions
/**
    Implementation of Algorithm "CurveKnotInsertion" from
    "The NURBS book", page 151, by Piegl and Tiller
**/
void insert_knot(curve_arrays& Curve, const k3d::double_t u, const k3d::uint_t r)
{
	curve_arrays result;
	result.order = Curve.order;
	const k3d::uint_t nr_points = Curve.points.size();
	normalize_knot_vector(Curve.knots);

	k3d::mesh::knots_t::iterator knot_u = std::find_if(Curve.knots.begin(), Curve.knots.end(), find_first_knot_after(u));
	const k3d::uint_t s = std::count_if(Curve.knots.begin(), Curve.knots.end(), find_knot_multiplicity(u));

	//we found the first knot greater than u or we're at the end so thats our k now
	int k = knot_u - Curve.knots.begin() - 1;

	if(s + r > Curve.order - 1)
	{
		k3d::log() << error << "insert_knot: No knot inserted: target multiplicity " << s + r << " for knot " << u << " exceeds curve degree " << Curve.order-1 << std::endl;
		return;
	}

	//*******************************************************************
	//Algorithm from "The NURBS book" page 151
	//*******************************************************************

	k3d::uint_t mp = Curve.knots.size();
	k3d::uint_t nq = nr_points + r;

	result.knots.insert(result.knots.end(), Curve.knots.begin(), knot_u);
	for (k3d::uint_t i = 1; i <= r; i++)
	{
		result.knots.push_back(u);
	}
	result.knots.insert(result.knots.end(), knot_u, Curve.knots.end());

	curve_copier result_input_copier(Curve, result);
	result.resize(nq, result.order);

	for (k3d::uint_t i = 0; i <= k + 1 - result.order; i++)
	{
		result_input_copier.copy(i, i);
	}

	for (k3d::uint_t i = k - s; i < nr_points; i++)
	{
		result_input_copier.copy(i, i + r);
	}

	curve_arrays tmp;
	curve_copier tmp_input_copier(Curve, tmp);
	curve_copier tmp_self_copier(tmp, tmp);
	curve_copier result_tmp_copier(tmp, result);
	for (k3d::uint_t i = 0; i <= Curve.order - 1 - s; i++)
	{
		tmp_input_copier.push_back(i + k - (Curve.order - 1));
	}
	k3d::uint_t L = 0;

	for (k3d::uint_t j = 1; j <= r; j++)
	{
		L = k - (Curve.order - 1) + j;
		double alpha = 0.0;
		for (k3d::uint_t i = 0; i <= Curve.order - 1 - j - s; i++)
		{
			alpha = (u - Curve.knots[L + i]) / (Curve.knots[i + k + 1] - Curve.knots[L + i]);
			k3d::uint_t indices[] = {static_cast<k3d::uint_t>(i + 1), static_cast<k3d::uint_t>(i)};
			k3d::double_t weights[] = {alpha, 1.0 - alpha};
			tmp_self_copier.copy(2, indices, weights, i);
		}
		result_tmp_copier.copy(0, L);
		result_tmp_copier.copy(Curve.order - 1 - j - s, k + r - j - s);
	}

	for (k3d::uint_t i = L + 1; i < k - s; i++)
	{
		result_tmp_copier.copy(i - L, i);
	}
	Curve = result;
}

k3d::bool_t is_closed(const k3d::nurbs_curve::const_primitive& NurbsCurves, const k3d::uint_t Curve)
{
	k3d::uint_t curve_points_begin = NurbsCurves.curve_first_points[Curve];
	k3d::uint_t curve_points_end = curve_points_begin + NurbsCurves.curve_point_counts[Curve];

	return (NurbsCurves.curve_points[curve_points_begin] == NurbsCurves.curve_points[curve_points_end - 1]);
}

void split_curve(k3d::mesh& OutputMesh, k3d::nurbs_curve::primitive& OutputCurves, const k3d::mesh& InputMesh, const k3d::nurbs_curve::const_primitive& InputCurves, k3d::uint_t Curve, const k3d::double_t u)
{
	// Can't split at end
	if(u > 1 - 0.000001 || u < 0.000001)
	{
		copy_curve(OutputMesh, OutputCurves, InputMesh, InputCurves, Curve);
		if(OutputCurves.material.empty())
			OutputCurves.material.push_back(InputCurves.material.back());
		return;
	}

	// Curve arrays for the original curve
	curve_arrays input_curve(InputMesh, InputCurves, Curve, true);

	// insert new knots at the requested u-value until the curve interpolates it
	const k3d::uint_t order = input_curve.order;
	const k3d::uint_t mul = multiplicity(input_curve.knots, u, 0, input_curve.knots.size());
	if(mul < (order - 1))
	{
		insert_knot(input_curve, u, order - mul - 1);
	}
	// locate the first knot after the u-value
	const k3d::mesh::knots_t::iterator split_knot = std::find_if(input_curve.knots.begin(), input_curve.knots.end(), find_first_knot_after(u));

	if(is_closed(InputCurves, Curve))
	{
		curve_arrays new_curve;
		new_curve.order = input_curve.order;
		new_curve.knots = input_curve.knots;
		curve_copier copier(input_curve, new_curve);
		const k3d::uint_t split_point = (split_knot - input_curve.knots.begin()) - order;
		const k3d::uint_t points_end = input_curve.points.size();
		for(k3d::uint_t i = split_point; i != points_end; ++i)
			copier.push_back(i);
		for(k3d::uint_t i = 1; i != split_point + 1; ++i)
			copier.push_back(i);

		new_curve.add_curve(OutputMesh, OutputCurves);
	}
	else
	{
		// First curve: all knots up to and including the last knot with multiplicity u
		curve_arrays first_curve;
		first_curve.order = input_curve.order;
		curve_copier first_copier(input_curve, first_curve);

		first_curve.knots.insert(first_curve.knots.begin(), input_curve.knots.begin(), split_knot);
		first_curve.knots.push_back(first_curve.knots.back()); // Clamp knot vector by increasing the multiplicity to become equal to the order
		const k3d::uint_t points_end = first_curve.knots.size() - order;
		for(k3d::uint_t i = 0; i != points_end; ++i)
			first_copier.push_back(i);

		// Second curve: start with order times the u-knot, then take all remaining knots
		curve_arrays second_curve;
		second_curve.order = input_curve.order;
		curve_copier second_copier(input_curve, second_curve);

		second_curve.knots.insert(second_curve.knots.end(), order, u);
		second_curve.knots.insert(second_curve.knots.end(), split_knot, input_curve.knots.end());
		const k3d::uint_t points_begin = first_curve.knots.size() - order - 1;
		for(k3d::uint_t i = points_begin; i != input_curve.points.size(); ++i)
			second_copier.push_back(i);

		// Parameter attributes need tweaking
		const k3d::double_t param_weights[] = {u, 1.0-u};
		const k3d::uint_t param_indices[] = {0, 1};
		first_curve.parameter_attributes = input_curve.parameter_attributes.clone_types();
		k3d::table_copier first_param_copier(input_curve.parameter_attributes, first_curve.parameter_attributes);
		first_param_copier.push_back(0);
		first_param_copier.push_back(2, param_indices, param_weights);
		second_curve.parameter_attributes = input_curve.parameter_attributes.clone_types();
		k3d::table_copier second_param_copier(input_curve.parameter_attributes, second_curve.parameter_attributes);
		second_param_copier.push_back(2, param_indices, param_weights);
		second_param_copier.push_back(1);

		first_curve.add_curve(OutputMesh, OutputCurves);
		second_curve.add_curve(OutputMesh, OutputCurves);
	}
	if(OutputCurves.material.empty())
		OutputCurves.material.push_back(InputCurves.material.back());
}

void insert_knot(k3d::mesh& OutputMesh, k3d::nurbs_curve::primitive& OutputCurves, const k3d::mesh& InputMesh, const k3d::nurbs_curve::const_primitive& InputCurves, k3d::uint_t Curve, const k3d::double_t u, const k3d::uint_t r)
{
	curve_arrays curve(InputMesh, InputCurves, Curve);
	insert_knot(curve, u, r);
	curve.add_curve(OutputMesh, OutputCurves);
}

void append_common_knot_vector(k3d::mesh::knots_t& CommonKnotVector, const k3d::nurbs_curve::const_primitive& NurbsCurves, const k3d::uint_t Curve)
{
	k3d::mesh::knots_t::const_iterator knots_begin = NurbsCurves.curve_knots.begin() + NurbsCurves.curve_first_knots[Curve];
	k3d::mesh::knots_t::const_iterator knots_end = knots_begin + NurbsCurves.curve_point_counts[Curve] + NurbsCurves.curve_orders[Curve];
	k3d::mesh::knots_t normalized_knots(knots_end - knots_begin);
	std::transform(knots_begin, knots_end, normalized_knots.begin(), knot_normalizer(*knots_begin, *(knots_end - 1)));

	for(k3d::mesh::knots_t::iterator knot = normalized_knots.begin(); knot != normalized_knots.end();)
	{
		const k3d::uint_t old_mul = std::count_if(CommonKnotVector.begin(), CommonKnotVector.end(), find_knot_multiplicity(*knot));
		const k3d::uint_t new_mul = std::count_if(knot, normalized_knots.end(), find_knot_multiplicity(*knot));
		if(new_mul > old_mul)
		{
			k3d::mesh::knots_t::iterator insert_loc = std::find_if(CommonKnotVector.begin(), CommonKnotVector.end(), find_first_knot_after(*knot));
			CommonKnotVector.insert(insert_loc, new_mul - old_mul, *knot);
		}
		knot += new_mul;
	}
}

const k3d::point3 centroid(const k3d::mesh::points_t& Points, const k3d::nurbs_curve::const_primitive& Curves, const k3d::uint_t Curve)
{
	k3d::point3 result(0,0,0);
	const k3d::uint_t points_begin = Curves.curve_first_points[Curve];
	const k3d::uint_t points_end = points_begin + Curves.curve_point_counts[Curve];
	k3d::mesh::bools_t used_points(Points.size(), false);
	for(k3d::uint_t point_idx = points_begin; point_idx != points_end; ++point_idx)
	{
		const k3d::uint_t point = Curves.curve_points[point_idx];
		if(!used_points[point])
			result += k3d::to_vector(Points[point]);
		used_points[point] = true;
	}
	return result / (points_end - points_begin);
}

void delete_selected_curves(k3d::mesh& Mesh)
{
	for(k3d::uint_t prim_idx = 0; prim_idx != Mesh.primitives.size(); ++prim_idx)
	{
		boost::scoped_ptr<k3d::nurbs_curve::primitive> curves(k3d::nurbs_curve::validate(Mesh, Mesh.primitives[prim_idx]));
		if(curves.get())
		{
			for(k3d::uint_t curve = 0; ;)
			{
				if(curves->curve_selections[curve])
					delete_curve(*curves, curve);
				else
					++curve;
				if(curve == curves->curve_selections.size())
					break;
			}
		}
	}
}

void straight_line(const k3d::point3& Start, const k3d::point3 End, const k3d::uint_t Segments, k3d::nurbs_curve::primitive& NurbsCurves, k3d::mesh& OutputMesh, const k3d::uint_t Order)
{
	k3d::vector3 delta = (End - Start) / Segments;

	k3d::mesh::points_t points;
	for (k3d::uint_t i = 0; i <= Segments; i++)
	{
		points.push_back(Start + delta * i);
	}
	add_curve(OutputMesh, NurbsCurves, Order, points);
}

void basis_functions(k3d::mesh::knots_t& BasisFunctions, const k3d::mesh::knots_t& Knots, const k3d::uint_t Order, const k3d::double_t U)
{
	BasisFunctions.assign(Order, 0.0);
	BasisFunctions[0] = 1;
	k3d::mesh::weights_t left(Order, 0.0);
	k3d::mesh::weights_t right(Order, 0.0);

	k3d::uint_t span = std::find_if(Knots.begin(), Knots.end(), find_first_knot_after(U)) - Knots.begin() - 1;
	if(span == (Knots.size() - 1))
		span -= Order;

	for (k3d::uint_t j = 1; j != Order; j++)
	{
		left[j] = U - Knots[span + 1 - j];
		right[j] = Knots[span + j] - U;

		k3d::double_t saved = 0.0;

		for (k3d::uint_t r = 0; r != j; r++)
		{
			k3d::double_t temp = BasisFunctions[r] / (right[r + 1] + left[j - r]);
			BasisFunctions[r] = saved + right[r + 1] * temp;
			saved = left[j - r] * temp;
		}
		BasisFunctions[j] = saved;
	}
}

/// Replace the elements of Array with their cumulative sum
template<class ArrayT> void cumulative_sum(ArrayT& Array)
{
	const k3d::uint_t array_begin = 0;
	const k3d::uint_t array_end = Array.size();
	for(k3d::uint_t i = array_begin + 1; i != array_end; ++i)
		Array[i] += Array[i-1];
}

void approximate(k3d::mesh::points_t& Points, k3d::mesh::weights_t& Weights, const k3d::mesh::knots_t& SampleParameters, const points4_t& SamplePoints, const k3d::uint_t Order, const k3d::mesh::knots_t& Knots)
{
	const k3d::uint_t knot_count = Knots.size();
	const k3d::uint_t dim = knot_count - Order; // The dimension of the problem to solve is equal to the number of control points of the final curve
	const k3d::uint_t sample_count = SampleParameters.size();

	std::vector<k3d::mesh::knots_t> sample_bases(sample_count); // Non-zero basis function values for each sample
	k3d::mesh::counts_t offsets; // Multiplicity of knots separating spans
	k3d::mesh::indices_t span_first_samples; // First sample point in a span between different knot values.
	k3d::mesh::counts_t span_sample_counts; // Number of samples in the given span.
	k3d::mesh::indices_t base_span_starts(Order, 0); // First span for each base
	k3d::mesh::indices_t base_span_ends; // Last span + 1 for each base

	offsets.push_back(0); // The multiplicity of the first knot is not counted
	span_first_samples.push_back(0);
	k3d::uint_t next_knot_idx = Order;
	k3d::double_t next_knot_val = Knots[next_knot_idx];
	for(k3d::uint_t i = 0; i != sample_count; ++i)
	{
		if(SampleParameters[i] > next_knot_val)
		{
			const k3d::uint_t mul = multiplicity(Knots, next_knot_val, 0, knot_count);
			offsets.push_back(mul);
			for(k3d::uint_t base = 0; base != mul; ++base)
			{
				base_span_starts.push_back(span_first_samples.size());
				base_span_ends.push_back(span_first_samples.size());
			}
			span_sample_counts.push_back(i - span_first_samples.back());
			span_first_samples.push_back(i);
			next_knot_idx += mul;
			next_knot_val = Knots[next_knot_idx];
		}
		basis_functions(sample_bases[i], Knots, Order, SampleParameters[i]);
	}
	span_sample_counts.push_back(sample_count - span_first_samples.back());
	base_span_ends.resize(dim, span_sample_counts.size());
	cumulative_sum(offsets);
	ublas::vector<k3d::double_t> rhs_x(dim);
	ublas::vector<k3d::double_t> rhs_y(dim);
	ublas::vector<k3d::double_t> rhs_z(dim);
	ublas::vector<k3d::double_t> rhs_w(dim);
	ublas::compressed_matrix<k3d::double_t> A(dim, dim, ((Order - 1)*2+1)*dim); // system matrix

	// Find out what control points would interpolate the curve
	k3d::mesh::bools_t interpolating_points(dim, false);
	points4_t interpolation_points(dim, k3d::point4(0,0,0,1)); // store the samples to interpolate through
	for(k3d::uint_t knot_idx = 0; knot_idx != knot_count;)
	{
		return_if_fail(knot_idx < knot_count);
		const k3d::uint_t mul = multiplicity(Knots, Knots[knot_idx], 0, knot_count);
		if(mul >= (Order - 1))
		{
			const k3d::double_t u = Knots[knot_idx];
			const k3d::uint_t sample_idx = (std::find_if(SampleParameters.begin(), SampleParameters.end(), find_first_knot_after(u)) - SampleParameters.begin()) - 1;
			return_if_fail(sample_idx < sample_count);
			knot_idx += mul;
			if(std::abs(SampleParameters[sample_idx] - u) < 0.00000001)
			{
				interpolating_points[knot_idx - Order - (knot_idx - Order == dim)] = true;
				interpolation_points[knot_idx - Order - (knot_idx - Order == dim)] = SamplePoints[sample_idx];
			}
		}
		else
		{
			++knot_idx;
		}
	}

	for(k3d::uint_t i = 0; i != dim; ++i)
	{
		if(interpolating_points[i])
		{
			A(i,i) = 1.0;
			const k3d::point4& p = interpolation_points[i];
			rhs_x(i) = p[0];
			rhs_y(i) = p[1];
			rhs_z(i) = p[2];
			rhs_w(i) = p[3];
		}
		else
		{
			const k3d::uint_t spans_begin_i = base_span_starts[i];
			const k3d::uint_t spans_end_i = base_span_ends[i];
			for(k3d::uint_t j = 0; j != dim; ++j)
			{
				k3d::double_t result = 0.0;
				const k3d::uint_t spans_begin = std::max(base_span_starts[j], spans_begin_i);
				const k3d::uint_t spans_end = std::min(base_span_ends[j], spans_end_i);
				for(k3d::uint_t span = spans_begin; span < spans_end; ++span)
				{
					const k3d::uint_t idx_i = i - offsets[span];
					const k3d::uint_t idx_j = j - offsets[span];
					const k3d::uint_t samples_begin = span_first_samples[span];
					const k3d::uint_t samples_end = samples_begin + span_sample_counts[span];
					for(k3d::uint_t sample = samples_begin; sample != samples_end; ++sample)
						result += sample_bases[sample][idx_i] * sample_bases[sample][idx_j];
				}
				A(i,j) = result;
			}

			k3d::double_t result_x = 0.0;
			k3d::double_t result_y = 0.0;
			k3d::double_t result_z = 0.0;
			k3d::double_t result_w = 0.0;
			for(k3d::uint_t span = spans_begin_i; span < spans_end_i; ++span)
			{
				const k3d::uint_t idx = i - offsets[span];
				const k3d::uint_t samples_begin = span_first_samples[span];
				const k3d::uint_t samples_end = samples_begin + span_sample_counts[span];
				for(k3d::uint_t sample = samples_begin; sample != samples_end; ++sample)
				{
					result_x += sample_bases[sample][idx] * SamplePoints[sample][0];
					result_y += sample_bases[sample][idx] * SamplePoints[sample][1];
					result_z += sample_bases[sample][idx] * SamplePoints[sample][2];
					result_w += sample_bases[sample][idx] * SamplePoints[sample][3];
				}
			}
			rhs_x(i) = result_x;
			rhs_y(i) = result_y;
			rhs_z(i) = result_z;
			rhs_w(i) = result_w;
		}
	}

	// This solves the system, overwriting the RHS with the solution
	try
	{
		ublas::permutation_matrix<k3d::double_t> PM(dim);
		ublas::lu_factorize(A, PM);
		ublas::lu_substitute(A, PM, rhs_x);
		ublas::lu_substitute(A, PM, rhs_y);
		ublas::lu_substitute(A, PM, rhs_z);
		ublas::lu_substitute(A, PM, rhs_w);

		// Store the solution in the output arrays
		Points.clear();
		Weights.clear();
		for(k3d::uint_t i = 0; i != dim; ++i)
		{
			const k3d::double_t w = rhs_w(i);
			Points.push_back(k3d::point3(rhs_x(i)/w, rhs_y(i)/w, rhs_z(i)/w));
			Weights.push_back(w);
		}
	}
	catch(std::exception& E)
	{
		k3d::log() << error << "error solving system: " << E.what() << std::endl;
		k3d::log() << error << "matrix: " << A << std::endl;
	}
}

void polygonize(k3d::mesh& OutputMesh, k3d::linear_curve::primitive& OutputCurve, const k3d::mesh& InputMesh, const k3d::nurbs_curve::const_primitive& InputCurves, const k3d::uint_t Curve, const k3d::uint_t Samples)
{
	curve_arrays curve(InputMesh, InputCurves, Curve, true);
	k3d::mesh::knots_t sample_u_vals;
	sample(sample_u_vals, curve.knots, Samples);

	if(OutputCurve.parameter_attributes.empty())
	{
		OutputCurve.parameter_attributes = curve.parameter_attributes.clone_types();
		OutputCurve.parameter_attributes.set_row_count(2*OutputCurve.curve_first_points.size());
	}
	k3d::table_copier parameter_copier(curve.parameter_attributes, OutputCurve.parameter_attributes);
	if(!curve.parameter_attributes.empty())
	{
		parameter_copier.push_back(0);
		parameter_copier.push_back(1);
	}

	if(OutputCurve.curve_attributes.empty())
	{
		OutputCurve.curve_attributes = curve.curve_attributes.clone_types();
		OutputCurve.curve_attributes.set_row_count(2*OutputCurve.curve_first_points.size());
	}
	k3d::table_copier curve_copier(curve.curve_attributes, OutputCurve.curve_attributes);
	if(!curve.curve_attributes.empty()) curve_copier.push_back(0);

	OutputCurve.periodic.push_back(false);
	OutputCurve.material = InputCurves.material;
	OutputCurve.curve_first_points.push_back(OutputCurve.curve_points.size());
	OutputCurve.curve_point_counts.push_back(0);
	OutputCurve.curve_selections.push_back(InputCurves.curve_selections[Curve]);

	if(OutputMesh.point_attributes.empty())
		OutputMesh.point_attributes = curve.point_attributes.clone_types();
	if(OutputCurve.vertex_attributes.empty())
		OutputCurve.vertex_attributes = curve.vertex_attributes.clone_types();

	k3d::mesh::points_t& points = OutputMesh.points.writable();
	const k3d::uint_t samples_end = sample_u_vals.size();
	for(k3d::uint_t i = 0; i != samples_end; ++i)
	{
		curve_arrays::curve_value val = curve.evaluate(sample_u_vals[i]);
		OutputCurve.curve_points.push_back(points.size());
		OutputCurve.curve_point_counts.back()++;
		points.push_back(val.position());

		k3d::table_copier point_copier(val.point_attributes, OutputMesh.point_attributes);
		point_copier.push_back(0);
		k3d::table_copier vertex_copier(val.vertex_attributes, OutputCurve.vertex_attributes);
		vertex_copier.push_back(0);
	}
	OutputMesh.point_selection.writable().resize(points.size(), 1.0);
}

void sample(k3d::mesh::knots_t& Samples, const k3d::mesh::knots_t& Knots, const k3d::uint_t SampleCount)
{
	k3d::mesh::knots_t unique_knots;
	const k3d::uint_t knots_end = Knots.size();
	for(k3d::uint_t i = 0; i < knots_end;)
	{
		const k3d::double_t u = Knots[i];
		unique_knots.push_back(u);
		i += multiplicity(Knots, u, i, knots_end);
	}

	const k3d::uint_t unique_knots_end = unique_knots.size() - 1;
	for(k3d::uint_t i = 0; i != unique_knots_end; ++i)
	{
		const k3d::double_t start_u = unique_knots[i];
		const k3d::double_t step = (unique_knots[i+1] - start_u) / static_cast<k3d::double_t>(SampleCount + 1);
		Samples.push_back(start_u);
		for(k3d::uint_t sample = 1; sample < (SampleCount+1); ++sample)
		{
			Samples.push_back(start_u + static_cast<k3d::double_t>(sample) * step);
		}
	}
	Samples.push_back(unique_knots.back());
}

} //namespace nurbs

} //namespace module
