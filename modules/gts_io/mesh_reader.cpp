// K-3D
// Copyright (c) 1995-2004, Timothy M. Shead
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
	\brief Implements GTSMeshReader, which reads GNU Triangulated Surface (.gts) files
	\author Tom Browder (tbrowder@home.com)
	\author Romain Behar (romainbehar@yahoo.com)
	\author Tim Shead (tshead@k3d.com)
*/

#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/file_helpers.h>
#include <k3dsdk/gzstream.h>
#include <k3dsdk/material_sink.h>
#include <k3dsdk/mesh_reader.h>
#include <k3dsdk/node.h>
#include <k3dsdk/polyhedron.h>
#include <k3dsdk/string_modifiers.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace gts
{

namespace io
{

/// Retrieve a line from a GTS file, skipping comments and empty lines
void gts_line(std::istream& Stream, std::string& Buffer)
{
	for(k3d::getline(Stream, Buffer); Stream; k3d::getline(Stream, Buffer))
	{
		if(Buffer[0] == '#')
			continue;

		if(k3d::trim(Buffer).empty())
			continue;

		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// mesh_reader

class mesh_reader :
	public k3d::material_sink<k3d::mesh_reader<k3d::node > >
{
	typedef k3d::material_sink<k3d::mesh_reader<k3d::node > > base;

public:
	mesh_reader(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document)
	{
	}

	void on_load_mesh(const k3d::filesystem::path& Path, k3d::mesh& Output)
	{
		Output = k3d::mesh();

		k3d::filesystem::igzstream file(Path);
		if(!file)
			return;

		// Read point, edge, and triangle counts ...
		std::string buffer;
		gts_line(file, buffer);
		if(!file)
		{
			k3d::log() << error << "Empty GTS file: " << Path.native_console_string() << std::endl;
			return;
		}

		k3d::uint_t point_count = 0;
		k3d::uint_t edge_count = 0;
		k3d::uint_t triangle_count = 0;
		std::stringstream line_buffer(buffer);
		line_buffer >> point_count >> edge_count >> triangle_count;

		// Read points ...
		k3d::mesh::points_t& points = Output.points.create(new k3d::mesh::points_t(point_count));
		k3d::mesh::selection_t& point_selection = Output.point_selection.create(new k3d::mesh::selection_t(point_count, 0.0));

		for(k3d::uint_t i = 0; i != point_count; ++i)
		{
			gts_line(file, buffer);
			if(!file)
			{
				k3d::log() << error << "Unexpected end-of-file: " << Path.native_console_string() << std::endl;
				return;
			}

			k3d::point3 point;
			std::stringstream line_buffer(buffer);
			line_buffer >> point[0] >> point[1] >> point[2];

			points[i] = point;
		}

		// Read edges ...
		std::vector<k3d::uint_t> edge_from;
		std::vector<k3d::uint_t> edge_to;

		edge_from.reserve(edge_count);
		edge_to.reserve(edge_count);

		for(k3d::uint_t i = 0; i != edge_count; ++i)
		{
			gts_line(file, buffer);
			if(!file)
			{
				k3d::log() << error << "Unexpected end-of-file: " << Path.native_console_string() << std::endl;
				return;
			}

			k3d::uint_t from;
			k3d::uint_t to;
			std::stringstream line_buffer(buffer);
			line_buffer >> from >> to;

			edge_from.push_back(from - 1); // GTS indices are one-based
			edge_to.push_back(to - 1); // GTS indices are one-based
		}

		// Read faces ...
		boost::scoped_ptr<k3d::polyhedron::primitive> polyhedron(k3d::polyhedron::create(Output));

		polyhedron->shell_types.push_back(k3d::polyhedron::POLYGONS);

		polyhedron->face_shells.reserve(triangle_count);
		polyhedron->face_first_loops.reserve(triangle_count);
		polyhedron->face_loop_counts.reserve(triangle_count);
		polyhedron->face_selections.reserve(triangle_count);
		polyhedron->face_materials.reserve(triangle_count);
		polyhedron->loop_first_edges.reserve(triangle_count);
		polyhedron->clockwise_edges.reserve(3 * triangle_count);
		polyhedron->edge_selections.reserve(3 * triangle_count);
		polyhedron->vertex_points.reserve(3 * triangle_count);
		polyhedron->vertex_selections.reserve(3 * triangle_count);

		for(k3d::uint_t i = 0; i != triangle_count; ++i)
		{
			gts_line(file, buffer);
			if(!file)
			{
				k3d::log() << error << "Unexpected end-of-file: " << Path.native_console_string() << std::endl;
				return;
			}

			k3d::uint_t edge1;
			k3d::uint_t edge2;
//			k3d::uint_t edge3; This is totally redundant ... strange file format!

			std::stringstream line_buffer(buffer);
			line_buffer >> edge1 >> edge2 /* >> edge3 */;

			// Calculate a consistent order for triangle points ...
			const k3d::uint_t edge1_from = edge_from[edge1 - 1];
			const k3d::uint_t edge1_to = edge_to[edge1 - 1];
			const k3d::uint_t edge2_from = edge_from[edge2 - 1];
			const k3d::uint_t edge2_to = edge_to[edge2 - 1];

			// Determine triangle orientation
			k3d::uint_t point1;
			k3d::uint_t point2;
			k3d::uint_t point3;

			if(edge1_from == edge2_from)
			{
				point1 = edge1_to;
				point2 = edge1_from;
				point3 = edge2_to;
			}
			else if(edge1_from == edge2_to)
			{
				point1 = edge1_to;
				point2 = edge1_from;
				point3 = edge2_from;
			}
			else if(edge1_to == edge2_from)
			{
				point1 = edge1_from;
				point2 = edge1_to;
				point3 = edge2_to;
			}
			else //if(edge1_to == edge2_to)
			{
				point1 = edge1_from;
				point2 = edge1_to;
				point3 = edge2_from;
			}

			polyhedron->face_shells.push_back(0);
			polyhedron->face_first_loops.push_back(polyhedron->loop_first_edges.size());
			polyhedron->face_loop_counts.push_back(1);
			polyhedron->face_selections.push_back(0.0);
			polyhedron->face_materials.push_back(0);
			polyhedron->loop_first_edges.push_back(polyhedron->clockwise_edges.size());
			
			polyhedron->clockwise_edges.push_back(polyhedron->clockwise_edges.size() + 1);
			polyhedron->edge_selections.push_back(0.0);
			polyhedron->vertex_points.push_back(point1);
			polyhedron->vertex_selections.push_back(0.0);

			polyhedron->clockwise_edges.push_back(polyhedron->clockwise_edges.size() + 1);
			polyhedron->edge_selections.push_back(0.0);
			polyhedron->vertex_points.push_back(point2);
			polyhedron->vertex_selections.push_back(0.0);

			polyhedron->clockwise_edges.push_back(polyhedron->clockwise_edges.size() - 2);
			polyhedron->edge_selections.push_back(0.0);
			polyhedron->vertex_points.push_back(point3);
			polyhedron->vertex_selections.push_back(0.0);
		}
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<mesh_reader, k3d::interface_list<k3d::imesh_source> > factory(
			k3d::uuid(0x7dbddd53, 0xef4e9588, 0x2643acb3, 0x4ad62813),
			"GTSMeshReader",
			_("Mesh reader that reads external GNU Triangulated Surface (.gts) files"),
			"MeshReader");

		return factory;
	}
};

k3d::iplugin_factory& mesh_reader_factory()
{
	return mesh_reader::get_factory();
}

} // namespace io

} // namespace gts

} // namespace module

