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
	\author Timothy M. Shead (tshead@k-3d.com)
	\author Bart Janssens (bart.janssens@lid.kviv.be)
*/

#include <k3dsdk/table_copier.h>
#include <k3dsdk/metadata_keys.h>
#include <k3dsdk/parallel/blocked_range.h>
#include <k3dsdk/parallel/parallel_for.h>
#include <k3dsdk/parallel/threads.h>
#include <k3dsdk/polyhedron.h>
#include <k3dsdk/primitive_validation.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/string_cast.h>
#include <k3dsdk/triangulator.h>

#include <boost/scoped_ptr.hpp>

#include <functional>
#include <iterator>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace k3d
{

namespace polyhedron
{

/////////////////////////////////////////////////////////////////////////////////////////////
// const_primitive

const_primitive::const_primitive(
	const typed_array<int32_t>& ShellTypes,
	const mesh::indices_t& FaceShells,
	const mesh::indices_t& FaceFirstLoops,
	const mesh::counts_t& FaceLoopCounts,
	const mesh::selection_t& FaceSelections,
	const mesh::materials_t& FaceMaterials,
	const mesh::indices_t& LoopFirstEdges,
	const mesh::indices_t& ClockwiseEdges,
	const mesh::selection_t& EdgeSelections,
	const mesh::indices_t& VertexPoints,
	const mesh::selection_t& VertexSelections,
	const mesh::table_t& ConstantAttributes,
	const mesh::table_t& FaceAttributes,
	const mesh::table_t& EdgeAttributes,
	const mesh::table_t& VertexAttributes
		) :
	shell_types(ShellTypes),
	face_shells(FaceShells),
	face_first_loops(FaceFirstLoops),
	face_loop_counts(FaceLoopCounts),
	face_selections(FaceSelections),
	face_materials(FaceMaterials),
	loop_first_edges(LoopFirstEdges),
	clockwise_edges(ClockwiseEdges),
	edge_selections(EdgeSelections),
	vertex_points(VertexPoints),
	vertex_selections(VertexSelections),
	constant_attributes(ConstantAttributes),
	face_attributes(FaceAttributes),
	edge_attributes(EdgeAttributes),
	vertex_attributes(VertexAttributes)
{
}

const_primitive::const_primitive(const primitive& Primitive) :
	shell_types(Primitive.shell_types),
	face_shells(Primitive.face_shells),
	face_first_loops(Primitive.face_first_loops),
	face_loop_counts(Primitive.face_loop_counts),
	face_selections(Primitive.face_selections),
	face_materials(Primitive.face_materials),
	loop_first_edges(Primitive.loop_first_edges),
	clockwise_edges(Primitive.clockwise_edges),
	edge_selections(Primitive.edge_selections),
	vertex_points(Primitive.vertex_points),
	vertex_selections(Primitive.vertex_selections),
	constant_attributes(Primitive.constant_attributes),
	face_attributes(Primitive.face_attributes),
	edge_attributes(Primitive.edge_attributes),
	vertex_attributes(Primitive.vertex_attributes)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
// primitive

primitive::primitive(
	typed_array<int32_t>& ShellTypes,
	mesh::indices_t& FaceShells,
	mesh::indices_t& FaceFirstLoops,
	mesh::counts_t& FaceLoopCounts,
	mesh::selection_t& FaceSelections,
	mesh::materials_t& FaceMaterials,
	mesh::indices_t& LoopFirstEdges,
	mesh::indices_t& ClockwiseEdges,
	mesh::selection_t& EdgeSelections,
	mesh::indices_t& VertexPoints,
	mesh::selection_t& VertexSelections,
	mesh::table_t& ConstantAttributes,
	mesh::table_t& FaceAttributes,
	mesh::table_t& EdgeAttributes,
	mesh::table_t& VertexAttributes
		) :
	shell_types(ShellTypes),
	face_shells(FaceShells),
	face_first_loops(FaceFirstLoops),
	face_loop_counts(FaceLoopCounts),
	face_selections(FaceSelections),
	face_materials(FaceMaterials),
	loop_first_edges(LoopFirstEdges),
	clockwise_edges(ClockwiseEdges),
	edge_selections(EdgeSelections),
	vertex_points(VertexPoints),
	vertex_selections(VertexSelections),
	constant_attributes(ConstantAttributes),
	face_attributes(FaceAttributes),
	edge_attributes(EdgeAttributes),
	vertex_attributes(VertexAttributes)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create

primitive* create(mesh::primitive& GenericPrimitive)
{
	GenericPrimitive.type = "polyhedron";
	GenericPrimitive.structure.clear();
	GenericPrimitive.attributes.clear();

	primitive* const result = new primitive(
		GenericPrimitive.structure["shell"].create<typed_array<int32_t> >("shell_types"),
		GenericPrimitive.structure["face"].create<mesh::indices_t>("face_shells"),
		GenericPrimitive.structure["face"].create<mesh::indices_t>("face_first_loops"),
		GenericPrimitive.structure["face"].create<mesh::counts_t>("face_loop_counts"),
		GenericPrimitive.structure["face"].create<mesh::selection_t>("face_selections"),
		GenericPrimitive.structure["face"].create<mesh::materials_t>("face_materials"),
		GenericPrimitive.structure["loop"].create<mesh::indices_t>("loop_first_edges"),
		GenericPrimitive.structure["edge"].create<mesh::indices_t>("clockwise_edges"),
		GenericPrimitive.structure["edge"].create<mesh::selection_t>("edge_selections"),
		GenericPrimitive.structure["vertex"].create<mesh::indices_t>("vertex_points"),
		GenericPrimitive.structure["vertex"].create<mesh::selection_t>("vertex_selections"),
		GenericPrimitive.attributes["constant"],
		GenericPrimitive.attributes["face"],
		GenericPrimitive.attributes["edge"],
		GenericPrimitive.attributes["vertex"]
		);

	result->face_selections.set_metadata_value(metadata::key::role(), metadata::value::selection_role());
	result->edge_selections.set_metadata_value(metadata::key::role(), metadata::value::selection_role());
	result->vertex_points.set_metadata_value(metadata::key::domain(), metadata::value::point_indices_domain());
	result->vertex_selections.set_metadata_value(metadata::key::role(), metadata::value::selection_role());

	return result;
}

primitive* create(mesh& Mesh)
{
	mesh::primitive& generic_primitive = Mesh.primitives.create("polyhedron");
	return create(generic_primitive);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create

primitive* create(mesh& Mesh, const mesh::points_t& Vertices, const mesh::counts_t& VertexCounts, const mesh::indices_t& VertexIndices, imaterial* const Material)
{
	try
	{
		if(std::count_if(VertexCounts.begin(), VertexCounts.end(), std::bind2nd(std::less<uint_t>(), 3)))
		{
			throw std::runtime_error("each face must have three-or-more vertices.");
		}

		const uint_t expected_indices = std::accumulate(VertexCounts.begin(), VertexCounts.end(), 0);
		if(VertexIndices.size() != expected_indices)
		{
			std::ostringstream buffer;
			buffer << "expected [" << expected_indices << "] vertex indices, received [" << VertexIndices.size() << "]";
			throw std::runtime_error(buffer.str());
		}

		if(std::count_if(VertexIndices.begin(), VertexIndices.end(), std::bind2nd(std::greater_equal<uint_t>(), Vertices.size())))
		{
			throw std::runtime_error("vertex indices out-of-bounds");
		}

		// Append points to the mesh (create the arrays if they don't already exist) ...
		mesh::points_t& points = Mesh.points ? Mesh.points.writable() : Mesh.points.create();
		mesh::selection_t& point_selection = Mesh.point_selection ? Mesh.point_selection.writable() : Mesh.point_selection.create();

		const uint_t point_offset = points.size();
		points.insert(points.end(), Vertices.begin(), Vertices.end());
		point_selection.insert(point_selection.end(), Vertices.size(), 0.0);
		Mesh.point_attributes.set_row_count(points.size());

		// Append a new polyhedron to the mesh ...
		primitive* const polyhedron = create(Mesh);
		polyhedron->shell_types.push_back(POLYGONS);

		uint_t face_vertex = 0;
		const uint_t face_begin = 0;
		const uint_t face_end = face_begin + VertexCounts.size();
		for(uint_t face = face_begin; face != face_end; ++face)
		{
			polyhedron->face_shells.push_back(0);
			polyhedron->face_first_loops.push_back(polyhedron->loop_first_edges.size());
			polyhedron->face_loop_counts.push_back(1);
			polyhedron->face_selections.push_back(0.0);
			polyhedron->face_materials.push_back(Material);
			polyhedron->loop_first_edges.push_back(polyhedron->clockwise_edges.size());

			const uint_t vertex_begin = 0;
			const uint_t vertex_end = vertex_begin + VertexCounts[face];
			const uint_t loop_begin = polyhedron->clockwise_edges.size();
			for(uint_t vertex = vertex_begin; vertex != vertex_end; ++vertex, ++face_vertex)
			{
				polyhedron->vertex_points.push_back(point_offset + VertexIndices[face_vertex]);
				polyhedron->vertex_selections.push_back(0.0);
				polyhedron->clockwise_edges.push_back(polyhedron->clockwise_edges.size() + 1);
				polyhedron->edge_selections.push_back(0.0);
			}
			polyhedron->clockwise_edges.back() = loop_begin;
		}

		return polyhedron;
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// validate

const_primitive* validate(const mesh& Mesh, const mesh::primitive& Primitive)
{
	if(Primitive.type != "polyhedron")
		return 0;

	try
	{
		require_valid_primitive(Mesh, Primitive);

		const mesh::table_t& shell_structure = require_structure(Primitive, "shell");
		const mesh::table_t& face_structure = require_structure(Primitive, "face");
		const mesh::table_t& loop_structure = require_structure(Primitive, "loop");
		const mesh::table_t& edge_structure = require_structure(Primitive, "edge");
		const mesh::table_t& vertex_structure = require_structure(Primitive, "vertex");

		const mesh::table_t& constant_attributes = require_attributes(Primitive, "constant");
		const mesh::table_t& face_attributes = require_attributes(Primitive, "face");
		const mesh::table_t& edge_attributes = require_attributes(Primitive, "edge");
		const mesh::table_t& vertex_attributes = require_attributes(Primitive, "vertex");

		const typed_array<int32_t>& shell_types = require_array<typed_array<int32_t> >(Primitive, shell_structure, "shell_types");
		const mesh::indices_t& face_shells = require_array<mesh::indices_t>(Primitive, face_structure, "face_shells");
		const mesh::indices_t& face_first_loops = require_array<mesh::indices_t>(Primitive, face_structure, "face_first_loops");
		const mesh::counts_t& face_loop_counts = require_array<mesh::counts_t>(Primitive, face_structure, "face_loop_counts");
		const mesh::selection_t& face_selections = require_array<mesh::selection_t>(Primitive, face_structure, "face_selections");
		const mesh::materials_t& face_materials = require_array<mesh::materials_t>(Primitive, face_structure, "face_materials");
		const mesh::indices_t& loop_first_edges = require_array<mesh::indices_t>(Primitive, loop_structure, "loop_first_edges");
		const mesh::indices_t& clockwise_edges = require_array<mesh::indices_t>(Primitive, edge_structure, "clockwise_edges");
		const mesh::selection_t& edge_selections = require_array<mesh::selection_t>(Primitive, edge_structure, "edge_selections");
		const mesh::indices_t& vertex_points = require_array<mesh::indices_t>(Primitive, vertex_structure, "vertex_points");
		const mesh::selection_t& vertex_selections = require_array<mesh::selection_t>(Primitive, vertex_structure, "vertex_selections");

		require_metadata(Primitive, face_selections, "face_selections", metadata::key::role(), metadata::value::selection_role());
		require_metadata(Primitive, edge_selections, "edge_selections", metadata::key::role(), metadata::value::selection_role());
		require_metadata(Primitive, vertex_points, "vertex_points", metadata::key::domain(), metadata::value::point_indices_domain());
		require_metadata(Primitive, vertex_selections, "vertex_selections", metadata::key::role(), metadata::value::selection_role());

		require_table_row_count(Primitive, loop_structure, "loop", std::accumulate(face_loop_counts.begin(), face_loop_counts.end(), 0));
		require_table_row_count(Primitive, vertex_structure, "vertex", edge_structure.row_count());

		// Check for out-of-bound shell indices ...
		const uint_t face_begin = 0;
		const uint_t face_end = face_begin + face_shells.size();
		for(uint_t face = face_begin; face != face_end; ++face)
		{
			if(face_shells[face] >= shell_types.size())
			{
				log() << error << "face shell out-of-bounds for face " << face << ": " << face_shells[face] << " >= " << shell_types.size() << std::endl;
				return 0;
			}	
		}

		// Check for out-of-bound indices and infinite loops in our edge lists ...
		const uint_t loop_begin = 0;
		const uint_t loop_end = loop_begin + loop_first_edges.size();

		const uint_t edge_begin = 0;
		const uint_t edge_end = edge_begin + clockwise_edges.size();

		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			const uint_t first_edge = loop_first_edges[loop];
			if(first_edge >= edge_end)
			{
				log() << error << "loop first edge index out-of-bounds for loop " << loop << std::endl;
				return 0;
			}

			uint_t edge_slow = first_edge;
			uint_t edge_fast = first_edge;
			uint_t cycle_count = 0;
			while(true)
			{
				edge_slow = clockwise_edges[edge_slow];
				if(edge_slow >= edge_end)
				{
					log() << error << "clockwise edge index out-of-bounds for edge " << edge_slow << std::endl;
					return 0;
				}

				edge_fast = clockwise_edges[clockwise_edges[edge_fast]];
				if(edge_fast >= edge_end)
				{
					log() << error << "clockwise edge index out-of-bounds for edge " << edge_fast << std::endl;
					return 0;
				}

				if(edge_slow == edge_fast)
					++cycle_count;

				if(cycle_count > 2)
				{
					log() << error << "infinite loop at loop index " << loop << std::endl;
					return 0;
				}

				if(edge_slow == first_edge)
					break;
			}
		}

		return new const_primitive(shell_types, face_shells, face_first_loops, face_loop_counts, face_selections, face_materials, loop_first_edges, clockwise_edges, edge_selections, vertex_points, vertex_selections, constant_attributes, face_attributes, edge_attributes, vertex_attributes);
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

primitive* validate(const mesh& Mesh, mesh::primitive& Primitive)
{
	if(Primitive.type != "polyhedron")
		return 0;

	try
	{
		require_valid_primitive(Mesh, Primitive);

		mesh::table_t& shell_structure = require_structure(Primitive, "shell");
		mesh::table_t& face_structure = require_structure(Primitive, "face");
		mesh::table_t& loop_structure = require_structure(Primitive, "loop");
		mesh::table_t& edge_structure = require_structure(Primitive, "edge");
		mesh::table_t& vertex_structure = require_structure(Primitive, "vertex");

		mesh::table_t& constant_attributes = require_attributes(Primitive, "constant");
		mesh::table_t& face_attributes = require_attributes(Primitive, "face");
		mesh::table_t& edge_attributes = require_attributes(Primitive, "edge");
		mesh::table_t& vertex_attributes = require_attributes(Primitive, "vertex");

		typed_array<int32_t>& shell_types = require_array<typed_array<int32_t> >(Primitive, shell_structure, "shell_types");
		mesh::indices_t& face_shells = require_array<mesh::indices_t>(Primitive, face_structure, "face_shells");
		mesh::indices_t& face_first_loops = require_array<mesh::indices_t>(Primitive, face_structure, "face_first_loops");
		mesh::counts_t& face_loop_counts = require_array<mesh::counts_t>(Primitive, face_structure, "face_loop_counts");
		mesh::selection_t& face_selections = require_array<mesh::selection_t>(Primitive, face_structure, "face_selections");
		mesh::materials_t& face_materials = require_array<mesh::materials_t>(Primitive, face_structure, "face_materials");
		mesh::indices_t& loop_first_edges = require_array<mesh::indices_t>(Primitive, loop_structure, "loop_first_edges");
		mesh::indices_t& clockwise_edges = require_array<mesh::indices_t>(Primitive, edge_structure, "clockwise_edges");
		mesh::selection_t& edge_selections = require_array<mesh::selection_t>(Primitive, edge_structure, "edge_selections");
		mesh::indices_t& vertex_points = require_array<mesh::indices_t>(Primitive, vertex_structure, "vertex_points");
		mesh::selection_t& vertex_selections = require_array<mesh::selection_t>(Primitive, vertex_structure, "vertex_selections");

		require_metadata(Primitive, face_selections, "face_selections", metadata::key::role(), metadata::value::selection_role());
		require_metadata(Primitive, edge_selections, "edge_selections", metadata::key::role(), metadata::value::selection_role());
		require_metadata(Primitive, vertex_points, "vertex_points", metadata::key::domain(), metadata::value::point_indices_domain());
		require_metadata(Primitive, vertex_selections, "vertex_selections", metadata::key::role(), metadata::value::selection_role());

		require_table_row_count(Primitive, loop_structure, "loop", std::accumulate(face_loop_counts.begin(), face_loop_counts.end(), 0));
		require_table_row_count(Primitive, vertex_structure, "vertex", edge_structure.row_count());

		// Check for out-of-bound shell indices ...
		const uint_t face_begin = 0;
		const uint_t face_end = face_begin + face_shells.size();
		for(uint_t face = face_begin; face != face_end; ++face)
		{
			if(face_shells[face] >= shell_types.size())
			{
				log() << error << "face shell out-of-bounds for face " << face << ": " << face_shells[face] << " >= " << shell_types.size() << std::endl;
				return 0;
			}	
		}

		// Check for out-of-bound indices and infinite loops in our edge lists ...
		const uint_t loop_begin = 0;
		const uint_t loop_end = loop_begin + loop_first_edges.size();

		const uint_t edge_begin = 0;
		const uint_t edge_end = edge_begin + clockwise_edges.size();

		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			const uint_t first_edge = loop_first_edges[loop];
			if(first_edge >= edge_end)
			{
				log() << error << "loop first edge index out-of-bounds for loop " << loop << std::endl;
				return 0;
			}

			uint_t edge_slow = first_edge;
			uint_t edge_fast = first_edge;
			uint_t cycle_count = 0;
			while(true)
			{
				edge_slow = clockwise_edges[edge_slow];
				if(edge_slow >= edge_end)
				{
					log() << error << "clockwise edge index out-of-bounds for edge " << edge_slow << std::endl;
					return 0;
				}

				edge_fast = clockwise_edges[clockwise_edges[edge_fast]];
				if(edge_fast >= edge_end)
				{
					log() << error << "clockwise edge index out-of-bounds for edge " << edge_fast << std::endl;
					return 0;
				}

				if(edge_slow == edge_fast)
					++cycle_count;

				if(cycle_count > 2)
				{
					log() << error << "infinite loop at loop index " << loop << std::endl;
					return 0;
				}

				if(edge_slow == first_edge)
					break;
			}
		}

		return new primitive(shell_types, face_shells, face_first_loops, face_loop_counts, face_selections, face_materials, loop_first_edges, clockwise_edges, edge_selections, vertex_points, vertex_selections, constant_attributes, face_attributes, edge_attributes, vertex_attributes);
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

primitive* validate(const mesh& Mesh, pipeline_data<mesh::primitive>& Primitive)
{
  if(!Primitive.get())
    return 0;

  if(Primitive->type != "polyhedron")
    return 0;

  return validate(Mesh, Primitive.writable());
}

/////////////////////////////////////////////////////////////////////////////////////////////
// add_triangle

void add_triangle(mesh& Mesh, primitive& Polyhedron, const uint_t Shell, const uint_t V1, const uint_t V2, const uint_t V3, imaterial* const Material)
{
	return_if_fail(Shell < Polyhedron.shell_types.size());

	Polyhedron.face_shells.push_back(Shell);
	Polyhedron.face_first_loops.push_back(Polyhedron.loop_first_edges.size());
	Polyhedron.face_loop_counts.push_back(1);
	Polyhedron.face_selections.push_back(0);
	Polyhedron.face_materials.push_back(Material);

	Polyhedron.loop_first_edges.push_back(Polyhedron.clockwise_edges.size());

	Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
	Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
	Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() - 2);

	Polyhedron.edge_selections.push_back(0);
	Polyhedron.edge_selections.push_back(0);
	Polyhedron.edge_selections.push_back(0);

	Polyhedron.vertex_points.push_back(V1);
	Polyhedron.vertex_points.push_back(V2);
	Polyhedron.vertex_points.push_back(V3);

	Polyhedron.vertex_selections.push_back(0);
	Polyhedron.vertex_selections.push_back(0);
	Polyhedron.vertex_selections.push_back(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// add_quadrilateral

void add_quadrilateral(mesh& Mesh, primitive& Polyhedron, const uint_t Shell, const uint_t V1, const uint_t V2, const uint_t V3, const uint_t V4, imaterial* const Material)
{
	return_if_fail(Shell < Polyhedron.shell_types.size());

	Polyhedron.face_shells.push_back(Shell);
	Polyhedron.face_first_loops.push_back(Polyhedron.loop_first_edges.size());
	Polyhedron.face_loop_counts.push_back(1);
	Polyhedron.face_selections.push_back(0);
	Polyhedron.face_materials.push_back(Material);

	Polyhedron.loop_first_edges.push_back(Polyhedron.clockwise_edges.size());

	Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
	Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
	Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
	Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() - 3);

	Polyhedron.edge_selections.push_back(0);
	Polyhedron.edge_selections.push_back(0);
	Polyhedron.edge_selections.push_back(0);
	Polyhedron.edge_selections.push_back(0);

	Polyhedron.vertex_points.push_back(V1);
	Polyhedron.vertex_points.push_back(V2);
	Polyhedron.vertex_points.push_back(V3);
	Polyhedron.vertex_points.push_back(V4);

	Polyhedron.vertex_selections.push_back(0);
	Polyhedron.vertex_selections.push_back(0);
	Polyhedron.vertex_selections.push_back(0);
	Polyhedron.vertex_selections.push_back(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// add_face

void add_face(mesh& Mesh, primitive& Polyhedron, const uint_t Shell, const mesh::points_t& Vertices, imaterial* const Material)
{
	add_face(Mesh, Polyhedron, Shell, Vertices, std::vector<mesh::points_t>(), Material);
}

void add_face(mesh& Mesh, primitive& Polyhedron, const uint_t Shell, const mesh::points_t& Vertices, const std::vector<mesh::points_t>& Holes, imaterial* const Material)
{
	return_if_fail(Mesh.points);
	return_if_fail(Mesh.point_selection);

	return_if_fail(Shell < Polyhedron.shell_types.size());

	return_if_fail(Vertices.size() > 1);
	for(uint_t hole = 0; hole != Holes.size(); ++hole)
		return_if_fail(Holes[hole].size() > 1);

	mesh::points_t& points = Mesh.points.writable();
	mesh::selection_t& point_selection = Mesh.point_selection.writable();

	Polyhedron.face_shells.push_back(Shell);
	Polyhedron.face_first_loops.push_back(Polyhedron.loop_first_edges.size());
	Polyhedron.face_loop_counts.push_back(Holes.size() + 1);
	Polyhedron.face_selections.push_back(0);
	Polyhedron.face_materials.push_back(Material);

	Polyhedron.loop_first_edges.push_back(Polyhedron.clockwise_edges.size());
	for(mesh::points_t::const_iterator point = Vertices.begin(); point != Vertices.end(); ++point)
	{
		Polyhedron.vertex_points.push_back(points.size());
		Polyhedron.vertex_selections.push_back(0);
		Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
		Polyhedron.edge_selections.push_back(0);
		
		points.push_back(*point);
		point_selection.push_back(0);
	}
	Polyhedron.clockwise_edges.back() = Polyhedron.loop_first_edges.back();

	for(uint_t hole = 0; hole != Holes.size(); ++hole)
	{
		Polyhedron.loop_first_edges.push_back(Polyhedron.clockwise_edges.size());
		for(mesh::points_t::const_iterator point = Holes[hole].begin(); point != Holes[hole].end(); ++point)
		{
			Polyhedron.vertex_points.push_back(points.size());
			Polyhedron.vertex_selections.push_back(0);
			Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
			Polyhedron.edge_selections.push_back(0);
			
			points.push_back(*point);
			point_selection.push_back(0);
		}
		Polyhedron.clockwise_edges.back() = Polyhedron.loop_first_edges.back();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// add_grid

void add_grid(mesh& Mesh, primitive& Polyhedron, const uint_t Shell, const uint_t Rows, const uint_t Columns, imaterial* const Material)
{
	try
	{
		if(!Rows)
			throw std::runtime_error("Cannot create grid with zero rows.");

		if(!Columns)
			throw std::runtime_error("Cannot create grid with zero columns.");

		if(Shell >= Polyhedron.shell_types.size())
			throw std::runtime_error("Invalid shell.");

		const uint_t rows = Rows;
		const uint_t columns = Columns;
		const uint_t point_rows = rows + 1;
		const uint_t point_columns = columns + 1;

		// Append points to the mesh (create the arrays if they don't already exist) ...
		mesh::points_t& points = Mesh.points ? Mesh.points.writable() : Mesh.points.create();
		mesh::selection_t& point_selection = Mesh.point_selection ? Mesh.point_selection.writable() : Mesh.point_selection.create();

		const uint_t point_offset = points.size();
		points.insert(points.end(), point_rows * point_columns, point3());
		point_selection.insert(point_selection.end(), point_rows * point_columns, 0);
		Mesh.point_attributes.set_row_count(points.size());

		// Append faces to the polyhedron ...
		for(uint_t row = 0; row != rows; ++row)
		{
			for(uint_t column = 0; column != columns; ++column)
			{
				Polyhedron.face_shells.push_back(Shell);
				Polyhedron.face_first_loops.push_back(Polyhedron.loop_first_edges.size());
				Polyhedron.face_loop_counts.push_back(1);
				Polyhedron.face_selections.push_back(0);
				Polyhedron.face_materials.push_back(Material);

				Polyhedron.loop_first_edges.push_back(Polyhedron.clockwise_edges.size());

				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() - 3);

				Polyhedron.edge_selections.insert(Polyhedron.edge_selections.end(), 4, 0);

				Polyhedron.vertex_points.push_back(point_offset + ((row + 0) * point_columns) + column + 0);
				Polyhedron.vertex_points.push_back(point_offset + ((row + 0) * point_columns) + column + 1);
				Polyhedron.vertex_points.push_back(point_offset + ((row + 1) * point_columns) + column + 1);
				Polyhedron.vertex_points.push_back(point_offset + ((row + 1) * point_columns) + column + 0);

				Polyhedron.vertex_selections.insert(Polyhedron.vertex_selections.end(), 4, 0);
			}
		}
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// add_cylinder

void add_cylinder(mesh& Mesh, primitive& Polyhedron, const uint_t Shell, const uint_t Rows, const uint_t Columns, imaterial* const Material)
{
	try
	{
		if(!Rows)
			throw std::runtime_error("Cannot create cylinder with zero rows.");

		if(Columns < 2)
			throw std::runtime_error("Cannot create cylinder with <2 columns.");

		if(Shell >= Polyhedron.shell_types.size())
			throw std::runtime_error("Invalid shell.");

		const uint_t rows = Rows;
		const uint_t columns = Columns;
		const uint_t point_rows = rows + 1;
		const uint_t point_columns = columns;

		// Append points to the mesh (create the arrays if they don't already exist) ...
		mesh::points_t& points = Mesh.points ? Mesh.points.writable() : Mesh.points.create();
		mesh::selection_t& point_selection = Mesh.point_selection ? Mesh.point_selection.writable() : Mesh.point_selection.create();

		const uint_t point_offset = points.size();
		points.insert(points.end(), point_rows * point_columns, point3());
		point_selection.insert(point_selection.end(), point_rows * point_columns, 0);
		Mesh.point_attributes.set_row_count(points.size());

		// Append faces to the polyhedron ...
		for(uint_t row = 0; row != rows; ++row)
		{
			for(uint_t column = 0; column != columns; ++column)
			{
				Polyhedron.face_shells.push_back(Shell);
				Polyhedron.face_first_loops.push_back(Polyhedron.loop_first_edges.size());
				Polyhedron.face_loop_counts.push_back(1);
				Polyhedron.face_selections.push_back(0);
				Polyhedron.face_materials.push_back(Material);

				Polyhedron.loop_first_edges.push_back(Polyhedron.clockwise_edges.size());

				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() - 3);

				Polyhedron.edge_selections.insert(Polyhedron.edge_selections.end(), 4, 0);

				Polyhedron.vertex_points.push_back(point_offset + ((row + 0) * point_columns) + (column + 0) % point_columns);
				Polyhedron.vertex_points.push_back(point_offset + ((row + 0) * point_columns) + (column + 1) % point_columns);
				Polyhedron.vertex_points.push_back(point_offset + ((row + 1) * point_columns) + (column + 1) % point_columns);
				Polyhedron.vertex_points.push_back(point_offset + ((row + 1) * point_columns) + (column + 0) % point_columns);

				Polyhedron.vertex_selections.insert(Polyhedron.vertex_selections.end(), 4, 0);
			}
		}
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// add_torus

void add_torus(mesh& Mesh, primitive& Polyhedron, const uint_t Shell, const uint_t Rows, const uint_t Columns, imaterial* const Material)
{
	try
	{
		if(Rows < 2)
			throw std::runtime_error("Cannot create torus with <2 rows.");

		if(Columns < 2)
			throw std::runtime_error("Cannot create torus with <2 columns.");

		if(Shell >= Polyhedron.shell_types.size())
			throw std::runtime_error("Invalid shell.");

		const uint_t rows = Rows;
		const uint_t columns = Columns;
		const uint_t point_rows = rows;
		const uint_t point_columns = columns;

		// Append points to the mesh (create the arrays if they don't already exist) ...
		mesh::points_t& points = Mesh.points ? Mesh.points.writable() : Mesh.points.create();
		mesh::selection_t& point_selection = Mesh.point_selection ? Mesh.point_selection.writable() : Mesh.point_selection.create();

		const uint_t point_offset = points.size();
		points.insert(points.end(), point_rows * point_columns, point3());
		point_selection.insert(point_selection.end(), point_rows * point_columns, 0);
		Mesh.point_attributes.set_row_count(points.size());

		// Append faces to the polyhedron ...
		for(uint_t row = 0; row != rows; ++row)
		{
			for(uint_t column = 0; column != columns; ++column)
			{
				Polyhedron.face_shells.push_back(Shell);
				Polyhedron.face_first_loops.push_back(Polyhedron.loop_first_edges.size());
				Polyhedron.face_loop_counts.push_back(1);
				Polyhedron.face_selections.push_back(0);
				Polyhedron.face_materials.push_back(Material);

				Polyhedron.loop_first_edges.push_back(Polyhedron.clockwise_edges.size());

				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() + 1);
				Polyhedron.clockwise_edges.push_back(Polyhedron.clockwise_edges.size() - 3);

				Polyhedron.edge_selections.insert(Polyhedron.edge_selections.end(), 4, 0);

				Polyhedron.vertex_points.push_back(point_offset + (((row + 0) % point_rows) * point_columns) + (column + 0) % point_columns);
				Polyhedron.vertex_points.push_back(point_offset + (((row + 0) % point_rows) * point_columns) + (column + 1) % point_columns);
				Polyhedron.vertex_points.push_back(point_offset + (((row + 1) % point_rows) * point_columns) + (column + 1) % point_columns);
				Polyhedron.vertex_points.push_back(point_offset + (((row + 1) % point_rows) * point_columns) + (column + 0) % point_columns);

				Polyhedron.vertex_selections.insert(Polyhedron.vertex_selections.end(), 4, 0);
			}
		}
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// is_triangles

bool_t is_triangles(const const_primitive& Polyhedron)
{
	const uint_t face_begin = 0;
	const uint_t face_end = face_begin + Polyhedron.face_first_loops.size();
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		uint_t edge_count = 0;
		const uint_t first_edge = Polyhedron.loop_first_edges[Polyhedron.face_first_loops[face]];
		for(uint_t edge = first_edge; ; )
		{
			++edge_count;

			edge = Polyhedron.clockwise_edges[edge];
			if(edge == first_edge)
				break;
		}

		if(edge_count != 3)
			return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// is_solid

bool_t is_solid(const const_primitive& Polyhedron)
{
	// K-3D uses a split-edge data structure to represent polyhedra.
	// We test for solidity by counting the number of edges that
	// connect each pair of points in the polyhedron.  A polyhedron is 
	// solid if-and-only-if each pair of points is used by two edges.

	mesh::bools_t boundary_edges;
	mesh::indices_t adjacent_edges;
	create_edge_adjacency_lookup(Polyhedron.vertex_points, Polyhedron.clockwise_edges, boundary_edges, adjacent_edges);
	return std::find(boundary_edges.begin(), boundary_edges.end(), true) == boundary_edges.end();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// is_sds

bool_t is_sds(const const_primitive& Polyhedron)
{
	return Polyhedron.shell_types.size() && (Polyhedron.shell_types[0] == CATMULL_CLARK);
}

//////////////////////////////////////////////////////////////////////////////
// create_counterclockwise_edge_lookup

void create_counterclockwise_edge_lookup(const const_primitive& Polyhedron, mesh::indices_t& CounterclockwiseEdges)
{
	CounterclockwiseEdges.resize(Polyhedron.clockwise_edges.size());

	const uint_t edge_begin = 0;
	const uint_t edge_end = edge_begin + Polyhedron.clockwise_edges.size();
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
		CounterclockwiseEdges[Polyhedron.clockwise_edges[edge]] = edge;
}

//////////////////////////////////////////////////////////////////////////////
// center

const point3 center(const mesh::indices_t& VertexPoints, const mesh::indices_t& ClockwiseEdges, const mesh::points_t& Points, const uint_t EdgeIndex)
{
	point3 result(0, 0, 0);

	uint_t count = 0;
	for(uint_t edge = EdgeIndex; ; )
	{
		result += to_vector(Points[VertexPoints[edge]]);
		++count;

		edge = ClockwiseEdges[edge];
		if(edge == EdgeIndex)
			break;
	}

	if(count)
		result /= static_cast<double>(count);

	return result;
}

//////////////////////////////////////////////////////////////////////////////
// normal

const normal3 normal(const mesh::indices_t& VertexPoints, const mesh::indices_t& ClockwiseEdges, const mesh::points_t& Points, const uint_t EdgeIndex)
{
	// Calculates the normal for an edge loop using the summation method, which is more robust than the three-point methods (handles zero-length edges)
	normal3 result(0, 0, 0);

	for(uint_t edge = EdgeIndex; ; )
	{
		const point3& i = Points[VertexPoints[edge]];
		const point3& j = Points[VertexPoints[ClockwiseEdges[edge]]];

		result[0] += (i[1] + j[1]) * (j[2] - i[2]);
		result[1] += (i[2] + j[2]) * (j[0] - i[0]);
		result[2] += (i[0] + j[0]) * (j[1] - i[1]);

		edge = ClockwiseEdges[edge];
		if(edge == EdgeIndex)
			break;
	}

	return 0.5 * result;
}

const normal3 normal(const point3& i, const point3& j, const point3& k)
{
	normal3 result(0, 0, 0);

	result[0] += (i[1] + j[1]) * (j[2] - i[2]);
	result[1] += (i[2] + j[2]) * (j[0] - i[0]);
	result[2] += (i[0] + j[0]) * (j[1] - i[1]);

	result[0] += (j[1] + k[1]) * (k[2] - j[2]);
	result[1] += (j[2] + k[2]) * (k[0] - j[0]);
	result[2] += (j[0] + k[0]) * (k[1] - j[1]);

	result[0] += (k[1] + i[1]) * (i[2] - k[2]);
	result[1] += (k[2] + i[2]) * (i[0] - k[0]);
	result[2] += (k[0] + i[0]) * (i[1] - k[1]);

	return 0.5 * result;
}

void create_face_normal_lookup(const mesh& Mesh, const const_primitive& Polyhedron, mesh::normals_t& Normals)
{
	Normals.resize(Polyhedron.face_first_loops.size(), normal3(0, 0, 0));

	return_if_fail(Mesh.points);
	const mesh::points_t& points = *Mesh.points;

	const uint_t face_begin = 0;
	const uint_t face_end = face_begin + Polyhedron.face_first_loops.size();
        for(uint_t face = face_begin; face != face_end; ++face)
	{
		for(uint_t edge = Polyhedron.loop_first_edges[Polyhedron.face_first_loops[face]]; ; )
		{
			const point3& i = points[Polyhedron.vertex_points[edge]];
			const point3& j = points[Polyhedron.vertex_points[Polyhedron.clockwise_edges[edge]]];

			Normals[face][0] += 0.5 * (i[1] + j[1]) * (j[2] - i[2]);
			Normals[face][1] += 0.5 * (i[2] + j[2]) * (j[0] - i[0]);
			Normals[face][2] += 0.5 * (i[0] + j[0]) * (j[1] - i[1]);

			edge = Polyhedron.clockwise_edges[edge];
			if(edge == Polyhedron.loop_first_edges[Polyhedron.face_first_loops[face]])
				break;
		}
	}
}

namespace detail
{

/////////////////////////////////////////////////////////////////////////////////////////////
// find_companion_worker

class find_companion_worker
{
public:
	find_companion_worker(
		const mesh::indices_t& VertexPoints,
		const mesh::indices_t& ClockwiseEdges,
		const std::vector<mesh::indices_t>& PointEdges,
		mesh::bools_t& BoundaryEdges,
		mesh::indices_t& AdjacentEdges) :
			m_edge_points(VertexPoints),
			m_clockwise_edges(ClockwiseEdges),
			m_point_edges(PointEdges),
			m_boundary_edges(BoundaryEdges),
			m_adjacent_edges(AdjacentEdges)
		{
		}
	
	void operator()(const k3d::parallel::blocked_range<uint_t>& range) const
	{
		const uint_t edge_begin = range.begin();
		const uint_t edge_end = range.end();
		for(uint_t edge = edge_begin; edge != edge_end; ++edge)
		{
			const uint_t vertex1 = m_edge_points[edge];
			const uint_t vertex2 = m_edge_points[m_clockwise_edges[edge]];

			const mesh::indices_t& point_edges = m_point_edges[vertex2];
			const uint_t first_index = 0;
			const uint_t last_index = point_edges.size();
			m_adjacent_edges[edge] = edge;
			for(uint_t i = first_index; i != last_index; ++i)
			{
				const uint_t companion = point_edges[i];
				if(m_edge_points[m_clockwise_edges[companion]] == vertex1)
				{
					m_boundary_edges[edge] = false;
					m_adjacent_edges[edge] = companion;
					break;
				}
			}
		}
	}
	
private:
	const mesh::indices_t& m_edge_points;
	const mesh::indices_t& m_clockwise_edges;
	const std::vector<mesh::indices_t>& m_point_edges;
	mesh::bools_t& m_boundary_edges;
	mesh::indices_t& m_adjacent_edges;
};

/// Returns true if a and b are collinear, up to the give threshold
/**
 * The surfaces of the parallelogram formed by the projections of a and b in the
 * XY, XZ and YZ plane all need to be < Threshold for the points to be collinear
 */
const bool_t is_collinear(const vector3& a, const vector3& b, const double_t Threshold)
{
	if(std::abs(a[0]*b[1] - a[1]*b[0]) > Threshold)
		return false;
	if(std::abs(a[0]*b[2] - a[2]*b[0]) > Threshold)
		return false;
	if(std::abs(a[1]*b[2] - a[2]*b[1]) > Threshold)
		return false;
	return true;
}

///////////////////////////////////////////////////
// create_triangles (taken from the triangulate_faces plugin, put here for sharing with CGAL module.

class create_triangles :
	public triangulator
{
	typedef triangulator base;

public:
	mesh::primitive* process(const mesh& Input, const const_primitive& Polyhedron, mesh& Output)
	{
		// Allocate new data structures for our output ...
		input_polyhedron = &Polyhedron;

		mesh::primitive* const result = new mesh::primitive();
		output_polyhedron.reset(create(*result));

		output_points = &Output.points.create(new mesh::points_t(*Input.points));
		output_point_selection = &Output.point_selection.create(new mesh::selection_t(Input.points->size(), 0.0));

		// Setup copying of attribute arrays ...
		output_polyhedron->constant_attributes = input_polyhedron->constant_attributes;

		output_polyhedron->face_attributes = input_polyhedron->face_attributes.clone_types();
		face_attributes_copier.reset(new table_copier(input_polyhedron->face_attributes, output_polyhedron->face_attributes));

		output_polyhedron->edge_attributes = input_polyhedron->edge_attributes.clone_types();
		edge_attributes_copier.reset(new table_copier(input_polyhedron->edge_attributes, output_polyhedron->edge_attributes));

		output_polyhedron->vertex_attributes = input_polyhedron->vertex_attributes.clone_types();
		vertex_attributes_copier.reset(new table_copier(input_polyhedron->vertex_attributes, output_polyhedron->vertex_attributes));

		Output.point_attributes = Input.point_attributes.clone();
		point_attributes_copier.reset(new table_copier(Input.point_attributes, Output.point_attributes));

		// Create the output polyhedron ...
		const uint_t face_begin = 0;
		const uint_t face_end = face_begin + Polyhedron.face_first_loops.size();
		for(uint_t face = face_begin; face != face_end; ++face)
		{
			if(Polyhedron.face_selections[face])
			{
				base::process(
					*Input.points,
					Polyhedron.face_first_loops,
					Polyhedron.face_loop_counts,
					Polyhedron.loop_first_edges,
					Polyhedron.vertex_points,
					Polyhedron.clockwise_edges,
					face);
			}
			else
			{
				add_existing_face(face);
			}
		}

		output_polyhedron->shell_types.assign(input_polyhedron->shell_types.size(), POLYGONS);

		return result;
	}
	
private:
	void start_face(const uint_t Face)
	{
		current_face = Face;
	}

	void add_vertex(const point3& Coordinates, uint_t Vertices[4], uint_t Edges[4], double Weights[4], uint_t& NewVertex)
	{
		NewVertex = output_points->size();

		output_points->push_back(Coordinates);
		output_point_selection->push_back(0.0);

		point_attributes_copier->push_back(4, Vertices, Weights);

		new_edge_attributes[NewVertex] = new_edge_record(Edges, Weights);
	}

	void add_triangle(uint_t Vertices[3], uint_t Edges[3])
	{
		output_polyhedron->face_shells.push_back(input_polyhedron->face_shells[current_face]);
		output_polyhedron->face_first_loops.push_back(output_polyhedron->loop_first_edges.size());
		output_polyhedron->face_loop_counts.push_back(1);
		output_polyhedron->face_selections.push_back(1.0);
		output_polyhedron->face_materials.push_back(input_polyhedron->face_materials[current_face]);

		face_attributes_copier->push_back(current_face);

		output_polyhedron->loop_first_edges.push_back(output_polyhedron->clockwise_edges.size());
		output_polyhedron->vertex_points.push_back(Vertices[0]);
		output_polyhedron->vertex_points.push_back(Vertices[1]);
		output_polyhedron->vertex_points.push_back(Vertices[2]);
		output_polyhedron->vertex_selections.push_back(0.0);
		output_polyhedron->vertex_selections.push_back(0.0);
		output_polyhedron->vertex_selections.push_back(0.0);
		output_polyhedron->clockwise_edges.push_back(output_polyhedron->vertex_points.size() - 2);
		output_polyhedron->clockwise_edges.push_back(output_polyhedron->vertex_points.size() - 1);
		output_polyhedron->clockwise_edges.push_back(output_polyhedron->vertex_points.size() - 3);
		output_polyhedron->edge_selections.push_back(0.0);
		output_polyhedron->edge_selections.push_back(0.0);
		output_polyhedron->edge_selections.push_back(0.0);

		for(uint_t i = 0; i != 3; ++i)
		{
			if(new_edge_attributes.count(Vertices[i]))
			{
				edge_attributes_copier->push_back(4, new_edge_attributes[Vertices[i]].edges, new_edge_attributes[Vertices[i]].weights);
				vertex_attributes_copier->push_back(4, new_edge_attributes[Vertices[i]].edges, new_edge_attributes[Vertices[i]].weights);
			}
			else
			{
				edge_attributes_copier->push_back(Edges[i]);
				vertex_attributes_copier->push_back(Edges[i]);
			}
		}
	}

	void add_existing_face(const uint_t Face)
	{
		output_polyhedron->face_shells.push_back(input_polyhedron->face_shells[Face]);
		output_polyhedron->face_first_loops.push_back(output_polyhedron->loop_first_edges.size());
		output_polyhedron->face_loop_counts.push_back(input_polyhedron->face_loop_counts[Face]);
		output_polyhedron->face_selections.push_back(0.0);
		output_polyhedron->face_materials.push_back(input_polyhedron->face_materials[Face]);

		face_attributes_copier->push_back(Face);

		const uint_t loop_begin = input_polyhedron->face_first_loops[Face];
		const uint_t loop_end = loop_begin + input_polyhedron->face_loop_counts[Face];
		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			output_polyhedron->loop_first_edges.push_back(output_polyhedron->vertex_points.size());

			const uint_t first_edge = input_polyhedron->loop_first_edges[loop];
			const uint_t edge_offset = output_polyhedron->vertex_points.size() - first_edge;
			for(uint_t edge = first_edge; ;)
			{
				output_polyhedron->vertex_points.push_back(input_polyhedron->vertex_points[edge]);
				output_polyhedron->vertex_selections.push_back(0.0);
				output_polyhedron->clockwise_edges.push_back(input_polyhedron->clockwise_edges[edge] + edge_offset);
				output_polyhedron->edge_selections.push_back(0.0);
				edge_attributes_copier->push_back(edge);
				vertex_attributes_copier->push_back(edge);

				edge = input_polyhedron->clockwise_edges[edge];
				if(edge == first_edge)
					break;
			}
		}
	}

	const const_primitive* input_polyhedron;

	mesh::points_t* output_points;
	mesh::selection_t* output_point_selection;
	boost::scoped_ptr<primitive> output_polyhedron;

	boost::shared_ptr<table_copier> face_attributes_copier;
	boost::shared_ptr<table_copier> edge_attributes_copier;
	boost::shared_ptr<table_copier> vertex_attributes_copier;
	boost::shared_ptr<table_copier> point_attributes_copier;

	uint_t current_face;

	struct new_edge_record
	{
		new_edge_record()
		{
		}

		new_edge_record(uint_t Edges[4], double_t Weights[4])
		{
			std::copy(Edges, Edges + 4, edges);
			std::copy(Weights, Weights + 4, weights);
		}

		uint_t edges[4];
		double_t weights[4];
	};

	std::map<uint_t, new_edge_record> new_edge_attributes;
};

} // namespace detail

/////////////////////////////////////////////////////////////////////////////////////////////
// create_edge_adjacency_lookup

void create_point_out_edge_lookup(const mesh::indices_t& VertexPoints, const mesh::indices_t& ClockwiseEdges, std::vector<mesh::indices_t>& AdjacencyList)
{
	const uint_t edge_begin = 0;
	const uint_t edge_end = edge_begin + ClockwiseEdges.size();
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
	{
		AdjacencyList[VertexPoints[edge]].push_back(edge);
	}
}

void create_edge_adjacency_lookup(const mesh::indices_t& VertexPoints, const mesh::indices_t& ClockwiseEdges, mesh::bools_t& BoundaryEdges, mesh::indices_t& AdjacentEdges)
{
	const k3d::uint_t count = VertexPoints.empty() ? 0 : *std::max_element(VertexPoints.begin(), VertexPoints.end()) + 1;
	if(!count)
		return;
	std::vector<mesh::indices_t> point_edges(count);
	create_point_out_edge_lookup(VertexPoints, ClockwiseEdges, point_edges);

	BoundaryEdges.assign(VertexPoints.size(), true);
	AdjacentEdges.assign(VertexPoints.size(), 0);

	const uint_t edge_begin = 0;
	const uint_t edge_end = edge_begin + VertexPoints.size();
	
	// Making this parallel decreases running time by 20 % on a Pentium D. 
	k3d::parallel::parallel_for(
				k3d::parallel::blocked_range<uint_t>(edge_begin, edge_end, k3d::parallel::grain_size()),
				detail::find_companion_worker(VertexPoints, ClockwiseEdges, point_edges, BoundaryEdges, AdjacentEdges));
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create_edge_face_lookup

void create_edge_face_lookup(const const_primitive& Polyhedron, mesh::indices_t& EdgeFaces)
{
	EdgeFaces.assign(Polyhedron.clockwise_edges.size(), 0);

	const uint_t face_begin = 0;
	const uint_t face_end = face_begin + Polyhedron.face_first_loops.size();
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		const uint_t loop_begin = Polyhedron.face_first_loops[face];
		const uint_t loop_end = loop_begin + Polyhedron.face_loop_counts[face];
		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			const uint_t first_edge = Polyhedron.loop_first_edges[loop];
			for(uint_t edge = first_edge; ;)
			{
				EdgeFaces[edge] = face;

				edge = Polyhedron.clockwise_edges[edge];
				if(edge == first_edge)
					break;
			}
		}
	}
}

void create_loop_edge_count_lookup(const const_primitive& Polyhedron, mesh::counts_t& LoopEdgeCounts)
{
	LoopEdgeCounts.assign(Polyhedron.loop_first_edges.size(), 0);

	const uint_t loop_begin = 0;
	const uint_t loop_end = loop_begin + Polyhedron.loop_first_edges.size();
	for(uint_t loop = loop_begin; loop != loop_end; ++loop)
	{
		const uint_t first_edge = Polyhedron.loop_first_edges[loop];
		for(uint_t edge = first_edge; ;)
		{
			++LoopEdgeCounts[loop];

			edge = Polyhedron.clockwise_edges[edge];
			if(edge == first_edge)
				break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create_point_face_lookup

void create_point_face_lookup(const mesh::indices_t& FaceFirstLoops, const mesh::indices_t& FaceLoopCounts, const mesh::indices_t& LoopFirstEdges, const mesh::indices_t& VertexPoints, const mesh::indices_t& ClockwiseEdges, const mesh::points_t& Points, mesh::indices_t& PointFirstFaces, mesh::counts_t& PointFaceCounts, mesh::indices_t& PointFaces)
{
	log() << warning << k3d_file_reference << " is deprecated" << std::endl;

	std::vector<std::vector<uint_t> > adjacency_list(Points.size());

	const uint_t face_begin = 0;
	const uint_t face_end = face_begin + FaceFirstLoops.size();
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		const uint_t loop_begin = FaceFirstLoops[face];
		const uint_t loop_end = loop_begin + FaceLoopCounts[face];
		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			const uint_t first_edge = LoopFirstEdges[loop];
			for(uint_t edge = first_edge; ;)
			{
				adjacency_list[VertexPoints[edge]].push_back(face);

				edge = ClockwiseEdges[edge];
				if(edge == first_edge)
					break;
			}
		}
	}

	PointFirstFaces.assign(Points.size(), 0);
	PointFaceCounts.assign(Points.size(), 0);
	PointFaces.clear();

	const uint_t point_begin = 0;
	const uint_t point_end = point_begin + Points.size();
	for(uint_t point = point_begin; point != point_end; ++point)
	{
		PointFirstFaces[point] = PointFaces.size();
		PointFaceCounts[point] = adjacency_list[point].size();
		PointFaces.insert(PointFaces.end(), adjacency_list[point].begin(), adjacency_list[point].end());
	}
}

void create_point_face_lookup(const mesh& Mesh, const const_primitive& Polyhedron, std::vector<mesh::indices_t>& AdjacencyList)
{
	AdjacencyList.resize(Mesh.points->size());

	const uint_t face_begin = 0;
	const uint_t face_end = face_begin + Polyhedron.face_shells.size();
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		const uint_t loop_begin = Polyhedron.face_first_loops[face];
		const uint_t loop_end = loop_begin + Polyhedron.face_loop_counts[face];
		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			const uint_t first_edge = Polyhedron.loop_first_edges[loop];
			for(uint_t edge = first_edge; ;)
			{
				AdjacencyList[Polyhedron.vertex_points[edge]].push_back(face);

				edge = Polyhedron.clockwise_edges[edge];
				if(edge == first_edge)
					break;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create_point_out_edge_lookup

void create_point_out_edge_lookup(const mesh& Mesh, const const_primitive& Polyhedron, std::vector<mesh::indices_t>& AdjacencyList)
{
	AdjacencyList.resize(Mesh.points->size());
	create_point_out_edge_lookup(Polyhedron.vertex_points, Polyhedron.clockwise_edges, AdjacencyList);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create_point_in_edge_lookup

void create_point_in_edge_lookup(const mesh& Mesh, const const_primitive& Polyhedron, std::vector<mesh::indices_t>& AdjacencyList)
{
	AdjacencyList.resize(Mesh.points->size());

	const uint_t edge_begin = 0;
	const uint_t edge_end = edge_begin + Polyhedron.clockwise_edges.size();
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
	{
		AdjacencyList[Polyhedron.vertex_points[Polyhedron.clockwise_edges[edge]]].push_back(edge);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create_point_edge_lookup

void create_point_edge_lookup(const mesh& Mesh, const const_primitive& Polyhedron, std::vector<mesh::indices_t>& AdjacencyList)
{
	AdjacencyList.resize(Mesh.points->size());

	const uint_t edge_begin = 0;
	const uint_t edge_end = edge_begin + Polyhedron.clockwise_edges.size();
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
	{
		AdjacencyList[Polyhedron.vertex_points[edge]].push_back(edge);
		AdjacencyList[Polyhedron.vertex_points[Polyhedron.clockwise_edges[edge]]].push_back(edge);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create_point_edge_lookup

void create_point_edge_lookup(const mesh::indices_t& VertexPoints, mesh::indices_t& PointEdges, mesh::indices_t& PointFirstEdges, mesh::counts_t& PointEdgeCounts)
{
	log() << warning << k3d_file_reference << " is deprecated" << std::endl;

	if(PointEdgeCounts.empty())
		create_point_valence_lookup(0, VertexPoints, PointEdgeCounts);
	const uint_t point_count = PointEdgeCounts.size();
	mesh::counts_t found_edges(point_count, 0);
	PointFirstEdges.assign(point_count, 0); // first edge in point_edges for each point
	PointEdges.assign(VertexPoints.size(), 0);
	uint_t count = 0;
	for(uint_t point = 0; point != point_count; ++point)
	{
		PointFirstEdges[point] = count;
		count += PointEdgeCounts[point];
	}

	const uint_t edge_begin = 0;
	const uint_t edge_end = edge_begin + VertexPoints.size();
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
	{
		const uint_t point = VertexPoints[edge];
		PointEdges[PointFirstEdges[point] + found_edges[point]] = edge;
		++found_edges[point];
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create_point_valence_lookup

void create_point_valence_lookup(const uint_t PointCount, const mesh::indices_t& VertexPoints, mesh::counts_t& Valences)
{
	log() << warning << k3d_file_reference << " is deprecated" << std::endl;

	Valences.assign(PointCount, 0);
	
	// Add 1 for each edge that starts at a point
	uint_t edge_count = VertexPoints.size();
	for (uint_t edge = 0; edge != edge_count; ++edge)
	{
		const uint_t edge_point = VertexPoints[edge];
		if(edge_point >= Valences.size()) // In case PointCount was not known to the caller
			Valences.resize(edge_point + 1, 0);
		++Valences[edge_point];
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create_boundary_face_lookup

void create_boundary_face_lookup(const mesh::indices_t& FaceFirstLoops, const mesh::indices_t& FaceLoopCounts, const mesh::indices_t& LoopFirstEdges, const mesh::indices_t& ClockwiseEdges, const mesh::bools_t& BoundaryEdges, const mesh::indices_t& AdjacentEdges, mesh::bools_t& BoundaryFaces)
{
	BoundaryFaces.clear();
	BoundaryFaces.resize(FaceFirstLoops.size());
	
	const uint_t face_begin = 0;
	const uint_t face_end = face_begin + FaceFirstLoops.size();
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		const uint_t loop_begin = FaceFirstLoops[face];
		const uint_t loop_end = loop_begin + FaceLoopCounts[face];
		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			const uint_t first_edge = LoopFirstEdges[loop];
			for(uint_t edge = first_edge; ;)
			{
				if (BoundaryEdges[edge])
				{
					BoundaryFaces[face] = true;
					break;
				}

				edge = ClockwiseEdges[edge];
				if(edge == first_edge)
					break;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// mark_collinear_edges

void mark_collinear_edges(mesh::indices_t& RedundantEdges, const mesh::selection_t& EdgeSelection, const mesh::points_t& Points, const mesh::indices_t& VertexPoints, const mesh::indices_t& ClockwiseEdges, const mesh::counts_t& VertexValences, const mesh::bools_t& BoundaryEdges, const mesh::indices_t& AdjacentEdges, const double_t Threshold)
{
	for(uint_t edge = 0; edge != VertexPoints.size(); ++edge)
	{
		if(!EdgeSelection[edge])
			continue;
		const uint_t clockwise = ClockwiseEdges[edge];
		const uint_t p1_index = VertexPoints[edge];
		const uint_t p2_index = VertexPoints[clockwise];
		const uint_t p3_index = VertexPoints[ClockwiseEdges[clockwise]];
		// valence must be 2 for internal edges or 1 for boundary edges
		if(!((!BoundaryEdges[clockwise] && VertexValences[p2_index] == 2) || (BoundaryEdges[clockwise] && VertexValences[p2_index] == 1)))
			continue;
		
		const point3& p1 = Points[p1_index];
		const point3& p2 = Points[p2_index];
		const point3& p3 = Points[p3_index];
		
		if(detail::is_collinear(p1-p2, p1-p3, Threshold))
		{
			RedundantEdges.push_back(clockwise);
		}
	}
}

void mark_coplanar_edges(const mesh::indices_t& Companions,
		const mesh::bools_t& BoundaryEdges,
		const mesh::normals_t& Normals,
		const mesh::indices_t& EdgeFaces,
		const mesh::selection_t& FaceSelection,
		mesh::indices_t& RedundantEdges,
		const double_t Threshold)
{
	for(uint_t edge = 0; edge != Companions.size(); ++edge)
	{
		if(BoundaryEdges[edge])
			continue;
		
		const uint_t face = EdgeFaces[edge];
		if(!FaceSelection[face])
			continue;
		
		const uint_t companion = Companions[edge];
		const uint_t companion_face = EdgeFaces[companion];
		if(!FaceSelection[companion_face])
			continue;
		
		if((!Normals[face].length()) || (std::abs((Normals[face] * Normals[companion_face]) - 1) < Threshold))
			RedundantEdges.push_back(edge);
	}
}

////////////////////////////////////////////
// triangulate

mesh::primitive* triangulate(const mesh& Input, const const_primitive& Polyhedron, mesh& Output)
{
	return detail::create_triangles().process(Input, Polyhedron, Output);
}

////////////////////////////////////////////
// delete_components

void delete_components(const mesh& Mesh, primitive& Polyhedron, const mesh::bools_t& RemovePoints, mesh::bools_t& RemoveEdges, mesh::bools_t& RemoveLoops, mesh::bools_t& RemoveFaces)
{
	// Sanity checks ...
	return_if_fail(RemovePoints.size() == Mesh.points->size());
	return_if_fail(RemoveEdges.size() == Polyhedron.clockwise_edges.size());
	return_if_fail(RemoveLoops.size() == Polyhedron.loop_first_edges.size());
	return_if_fail(RemoveFaces.size() == Polyhedron.face_shells.size());

	// Cache some useful stuff up-front ...
	const uint_t face_begin = 0;
	const uint_t face_end = face_begin + Polyhedron.face_shells.size();

	const uint_t edge_begin = 0;
	const uint_t edge_end = edge_begin + Polyhedron.clockwise_edges.size();

	const uint_t loop_begin = 0;
	const uint_t loop_end = loop_begin + Polyhedron.loop_first_edges.size();

	// Mark edges to be implicitly removed because their points are going away ...
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
	{
		if(!RemovePoints[Polyhedron.vertex_points[edge]])
			continue;

		RemoveEdges[edge] = true;
	}

	// Mark loops to be implicitly removed because they have fewer than three edges remaining ...
	for(uint_t loop = loop_begin; loop != loop_end; ++loop)
	{
		const uint_t first_edge = Polyhedron.loop_first_edges[loop];

		uint_t remaining_edge_count = 0;
		for(uint_t edge = first_edge; ;)
		{
			if(!RemoveEdges[edge])
				++remaining_edge_count;

			edge = Polyhedron.clockwise_edges[edge];
			if(edge == first_edge)
				break;
		}

		if(remaining_edge_count < 3)
			RemoveLoops[loop] = true;
	}

	// Mark faces to be implicitly removed because their first loop is going away ...
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		if(RemoveFaces[face])
			continue;

		if(RemoveLoops[Polyhedron.face_first_loops[face]])
			RemoveFaces[face] = true;
	}

	// Mark loops and edges to be implicitly removed because their face is going away ...
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		if(!RemoveFaces[face])
			continue;

		const uint_t loop_begin = Polyhedron.face_first_loops[face];
		const uint_t loop_end = loop_begin + Polyhedron.face_loop_counts[face];
		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			RemoveLoops[loop] = true;

			const uint_t first_edge = Polyhedron.loop_first_edges[loop];
			for(uint_t edge = first_edge; ;)
			{
				RemoveEdges[edge] = true;

				edge = Polyhedron.clockwise_edges[edge];
				if(edge == first_edge)
					break;
			}
		}
	}

	// Compute new first edges for loops ...
	mesh::indices_t new_first_edge(Polyhedron.loop_first_edges.size());
	for(uint_t loop = loop_begin; loop != loop_end; ++loop)
	{
		if(RemoveLoops[loop])
			continue;

		const uint_t first_edge = Polyhedron.loop_first_edges[loop];
		for(uint_t edge = first_edge; ;)
		{
			if(!RemoveEdges[edge])
			{
				new_first_edge[loop] = edge;
				break;
			}

			edge = Polyhedron.clockwise_edges[edge];
			if(edge == first_edge)
				break;
		}
	}

	// Compute new clockwise_edges ...
	mesh::indices_t new_clockwise_edge(Polyhedron.clockwise_edges.size());
	for(uint_t loop = loop_begin; loop != loop_end; ++loop)
	{
		if(RemoveLoops[loop])
			continue;

		const uint_t first_edge = Polyhedron.loop_first_edges[loop];

		mesh::indices_t remaining_edges;
		for(uint_t edge = first_edge; ;)
		{
			if(!RemoveEdges[edge])
				remaining_edges.push_back(edge);

			edge = Polyhedron.clockwise_edges[edge];
			if(edge == first_edge)
				break;
		}

		for(uint_t e = 0; e != remaining_edges.size(); ++e)
		{
			new_clockwise_edge[remaining_edges[e]] = remaining_edges[(e+1)%remaining_edges.size()];
		}
	}

	// Compute maps and remaining component counts ...
	mesh::indices_t face_map;
	mesh::create_index_removal_map(RemoveFaces, face_map);
	const uint_t remaining_faces = std::count(RemoveFaces.begin(), RemoveFaces.end(), false);

	mesh::indices_t loop_map;
	mesh::create_index_removal_map(RemoveLoops, loop_map);
	const uint_t remaining_loops = std::count(RemoveLoops.begin(), RemoveLoops.end(), false);

	mesh::indices_t edge_map;
	mesh::create_index_removal_map(RemoveEdges, edge_map);
	const uint_t remaining_edges = std::count(RemoveEdges.begin(), RemoveEdges.end(), false);

	// Delete faces, updating loop indices as we go ...
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		if(RemoveFaces[face])
			continue;

		Polyhedron.face_shells[face_map[face]] = Polyhedron.face_shells[face];
		Polyhedron.face_first_loops[face_map[face]] = loop_map[Polyhedron.face_first_loops[face]];
		Polyhedron.face_loop_counts[face_map[face]] = Polyhedron.face_loop_counts[face];
		Polyhedron.face_selections[face_map[face]] = Polyhedron.face_selections[face];
		Polyhedron.face_materials[face_map[face]] = Polyhedron.face_materials[face];
	}
	Polyhedron.face_shells.resize(remaining_faces);
	Polyhedron.face_first_loops.resize(remaining_faces);
	Polyhedron.face_loop_counts.resize(remaining_faces);
	Polyhedron.face_selections.resize(remaining_faces);
	Polyhedron.face_materials.resize(remaining_faces);

	// Delete face attributes ...
	table_copier face_attribute_copier(Polyhedron.face_attributes);
	for(uint_t face = face_begin; face != face_end; ++face)
	{
		if(RemoveFaces[face])
			continue;

		face_attribute_copier.copy(face, face_map[face]);
	}
	Polyhedron.face_attributes.set_row_count(remaining_faces);

	// Delete loops, updating edge indices as we go ...
	for(uint_t loop = loop_begin; loop != loop_end; ++loop)
	{
		if(RemoveLoops[loop])
			continue;

		Polyhedron.loop_first_edges[loop_map[loop]] = edge_map[new_first_edge[loop]];
	}
	Polyhedron.loop_first_edges.resize(remaining_loops);

	// Delete edges, updating edge indices as we go ...
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
	{
		if(RemoveEdges[edge])
			continue;

		Polyhedron.clockwise_edges[edge_map[edge]] = edge_map[new_clockwise_edge[edge]];
		Polyhedron.edge_selections[edge_map[edge]] = Polyhedron.edge_selections[edge];
		Polyhedron.vertex_points[edge_map[edge]] = Polyhedron.vertex_points[edge];
		Polyhedron.vertex_selections[edge_map[edge]] = Polyhedron.vertex_selections[edge];
	}
	Polyhedron.clockwise_edges.resize(remaining_edges);
	Polyhedron.edge_selections.resize(remaining_edges);
	Polyhedron.vertex_points.resize(remaining_edges);
	Polyhedron.vertex_selections.resize(remaining_edges);

	// Delete edge attributes ...
	table_copier edge_attribute_copier(Polyhedron.edge_attributes);
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
	{
		if(RemoveEdges[edge])
			continue;

		edge_attribute_copier.copy(edge, edge_map[edge]);
	}
	Polyhedron.edge_attributes.set_row_count(remaining_edges);

	// Delete vertex attributes ...
	table_copier vertex_attribute_copier(Polyhedron.vertex_attributes);
	for(uint_t edge = edge_begin; edge != edge_end; ++edge)
	{
		if(RemoveEdges[edge])
			continue;

		vertex_attribute_copier.copy(edge, edge_map[edge]);
	}
	Polyhedron.vertex_attributes.set_row_count(remaining_edges);
}

} // namespace polyhedron

} // namespace k3d

