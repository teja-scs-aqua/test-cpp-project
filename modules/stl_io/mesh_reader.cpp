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
	\author Tim Shead (tshead@k-3d.com)
	\author Bart Janssens (bart.janssens@lid.kviv.be)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/file_helpers.h>
#include <k3dsdk/fstream.h>
#include <k3dsdk/mesh_reader.h>
#include <k3dsdk/node.h>
#include <k3dsdk/polyhedron.h>

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

#include <set>

#include "binary_stl.h"

namespace module
{

namespace stl
{

namespace io
{

namespace detail
{

/// True if the supplied file is an ASCII STL file
k3d::bool_t is_ascii(std::istream& Stream)
{
	char buffer[80];
	Stream.read(buffer, 80);
	Stream.seekg(0, std::ios::beg);
	return (boost::algorithm::starts_with(buffer, "solid"));
}

struct compare_points
{
	bool operator()(const k3d::point3& A, const k3d::point3& B) const
	{
		if (A[0] != B[0])
		{
			return A[0] < B[0];
		}
		if (A[1] != B[1])
		{
			return A[1] < B[1];
		}
		return A[2] < B[2];
	}
};

typedef std::map<k3d::point3, k3d::uint_t, compare_points> point_map_t;

void fill_points(const point_map_t& PointMap, k3d::mesh::points_t& Points)
{
	Points.resize(PointMap.size());
	for(point_map_t::const_iterator it = PointMap.begin(); it != PointMap.end(); ++it)
	{
		Points[it->second] = it->first;
	}
}

/// Adds a point without introducing duplicates
k3d::uint_t add_point(point_map_t &PointMap, const k3d::point3 &Point)
{
	return PointMap.insert(std::make_pair(Point, PointMap.size())).first->second;
}

/// Extracts the STL topology information, merging points that are less than threshold apart
void get_stl_topology(std::istream& Stream, k3d::mesh::points_t& Points, k3d::mesh::counts_t& VertexCounts, k3d::mesh::indices_t& VertexIndices, k3d::mesh::normals_t& Normals, const k3d::double_t Threshold = 1e-12)
{
	const k3d::double_t threshold = Threshold*Threshold;

	detail::point_map_t point_map;

	k3d::string_t line_buffer;
	k3d::uint_t line_number = 0;
	k3d::mesh::indices_t face_points;
	k3d::normal3 face_normal;
	std::set<k3d::string_t> added_faces; // stores a unique ID for each added face
	for(k3d::getline(Stream, line_buffer); Stream; k3d::getline(Stream, line_buffer))
	{
		++line_number;
		k3d::string_t keyword;
		std::istringstream line_stream(line_buffer);
		line_stream >> keyword;

		if(keyword == "facet")
		{
			k3d::string_t keyword2;
			line_stream >> keyword2;
			assert_warning(keyword2 == "normal");
			k3d::double_t x, y, z;
			line_stream >> x;
			line_stream >> y;
			line_stream >> z;
			face_normal = k3d::normalize(k3d::normal3(x,y,z));
		}
		if(keyword == "vertex")
		{
			k3d::double_t x, y, z;
			line_stream >> x;
			line_stream >> y;
			line_stream >> z;
			k3d::point3 new_point(x, y, z);

			face_points.push_back(detail::add_point(point_map, new_point));
			if(face_points.size() == 3)
			{
				std::stringstream face_id_stream;
				face_id_stream << face_points[0] << face_points[1] << face_points[2];
				k3d::string_t face_id = face_id_stream.str();
				if(added_faces.count(face_id))
				{
					k3d::log() << warning << "Skipping duplicate face on line " << line_number - 4 << std::endl;
				}
				else
				{
					VertexIndices.insert(VertexIndices.end(), face_points.begin(), face_points.end());
					VertexCounts.push_back(3);
					Normals.push_back(face_normal);
					added_faces.insert(face_id);
				}
				face_points.clear();
			}
		}
		if(keyword == "endfacet")
		{
			if(face_points.size())
			{
				std::stringstream error_stream;
				error_stream << "Error: STL file had less than 3 vertices for face ending on line " << line_number;
				throw std::runtime_error(error_stream.str());
			}
		}
	}
	detail::fill_points(point_map, Points);
}

const k3d::normal3 normal(const k3d::mesh::points_t Points, k3d::mesh::indices_t& VertexIndices, const k3d::uint_t FaceIndex)
{
	// Calculates the normal for an edge loop using the summation method, which is more robust than the three-point methods (handles zero-length edges)
	k3d::normal3 result(0, 0, 0);

	const k3d::uint_t face_start = 3*FaceIndex;
	const k3d::uint_t face_end = face_start + 3;
	for(k3d::uint_t point = face_start; point != face_end; ++point)
	{
		const k3d::point3& i = Points[VertexIndices[point]];
		const k3d::point3& j = Points[VertexIndices[(point+1) == face_end ? face_start : point + 1]];

		result[0] += (i[1] + j[1]) * (j[2] - i[2]);
		result[1] += (i[2] + j[2]) * (j[0] - i[0]);
		result[2] += (i[0] + j[0]) * (j[1] - i[1]);
	}

	return 0.5 * result;
}

/// Make the face orientation consistent with the normal stored on file
void adjust_orientation(const k3d::mesh::points_t Points, k3d::mesh::indices_t& VertexIndices, const k3d::mesh::normals_t& Normals)
{
	for(k3d::uint_t face = 0; face != Normals.size(); ++face)
	{
		const k3d::normal3 calculated_normal = k3d::normalize(normal(Points, VertexIndices, face));
		const k3d::normal3& stored_normal = Normals[face];
		const k3d::uint_t face_start = face * 3;
		if((calculated_normal * stored_normal) < 0)
		{
			// stored normal is opposite to face rientation, so we flip face orientation
			const k3d::uint_t old_first_point = VertexIndices[face_start];
			VertexIndices[face_start] = VertexIndices[face_start + 1];
			VertexIndices[face_start + 1] = old_first_point;
		}
	}
}

/// 2-byte integer value to a K-3D color
k3d::color convert_color_viscam(const k3d::uint16_t Color, const k3d::color& BaseColor)
{
	k3d::uint16_t color = switch_bytes(Color);
	const k3d::uint16_t blue = color >> 11;
	const k3d::uint16_t green = (color - (blue << 11)) >> 6;
	const k3d::uint16_t red = (color - (blue << 11) - (green << 6)) >> 1;
	const k3d::uint16_t use_color = (color - (blue << 11) - (green << 6) - (red << 1));
	const k3d::color result = use_color ? k3d::color(static_cast<k3d::double_t>(red)/31., static_cast<k3d::double_t>(green)/31., static_cast<k3d::double_t>(blue)/31.) : BaseColor;
	return result;
}

/// 2-byte integer value to a K-3D color
k3d::color convert_color_magics(const k3d::uint16_t Color, const k3d::color& BaseColor)
{
	k3d::uint16_t color = switch_bytes(Color);
	const k3d::uint16_t red = color >> 11;
	const k3d::uint16_t green = (color - (red << 11)) >> 6;
	const k3d::uint16_t blue = (color - (red << 11) - (green << 6)) >> 1;
	const k3d::uint16_t use_color = (color - (red << 11) - (green << 6) - (blue << 1));
	const k3d::color result = !use_color ? k3d::color(static_cast<k3d::double_t>(red)/31., static_cast<k3d::double_t>(green)/31., static_cast<k3d::double_t>(blue)/31.) : BaseColor;
	return result;
}

} // namespace detail

/////////////////////////////////////////////////////////////////////////////
// mesh_reader

class mesh_reader :
	public k3d::mesh_reader<k3d::node >
{
	typedef k3d::mesh_reader<k3d::node > base;

public:
	mesh_reader(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_threshold(init_owner(*this) + init_name("threshold") + init_label(_("Threshold")) + init_description(_("Controls the sensitivity for deciding when two edges are collinear.")) + init_value(1e-8) + init_step_increment(1e-8) + init_units(typeid(k3d::measurement::scalar))),
		m_store_normals(init_owner(*this) + init_name("store_normals") + init_label(_("Store Normals")) + init_description(_("If true, the normals from the STL file are read into a face attribute array")) + init_value(false)),
		m_color_array(init_owner(*this) + init_name("color_array") + init_label(_("Color Array")) + init_description(_("Name of the array containing face colors (for binary, colored STL only)")) + init_value(std::string("Cs")))
	{
		m_threshold.changed_signal().connect(k3d::hint::converter<
						k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_reload_mesh_slot()));
		m_store_normals.changed_signal().connect(k3d::hint::converter<
						k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_reload_mesh_slot()));
	}

	void on_load_mesh(const k3d::filesystem::path& Path, k3d::mesh& Output)
	{
		Output = k3d::mesh();

		k3d::filesystem::ifstream file(Path);
		if(!file)
		{
			k3d::log() << error << k3d_file_reference << ": error opening [" << Path.native_console_string() << "]" << std::endl;
			return;
		}
		
		k3d::mesh::points_t points;
		k3d::mesh::counts_t vertex_counts;
		k3d::mesh::indices_t vertex_indices;
		k3d::mesh::normals_t face_normals;
		
		detail::point_map_t point_map;
		
		try
		{
			if(detail::is_ascii(file))
			{
				detail::get_stl_topology(file, points, vertex_counts, vertex_indices, face_normals, m_threshold.pipeline_value());
				detail::adjust_orientation(points, vertex_indices, face_normals);
				k3d::polyhedron::primitive* polyhedron = k3d::polyhedron::create(Output, points, vertex_counts, vertex_indices, static_cast<k3d::imaterial*>(0));
				if(m_store_normals.pipeline_value())
					polyhedron->face_attributes.create("N", new k3d::mesh::normals_t(face_normals));
			}
			else
			{
				const k3d::double_t threshold = m_threshold.pipeline_value();
				k3d::mesh::colors_t face_colors;
				binary_stl stl;
				stl.read(file);
				k3d::color base_color(0.8, 0.8, 0.8);
				k3d::bool_t is_magics = false;
				if(boost::algorithm::contains(stl.header, "COLOR="))
				{
					k3d::uint8_t* color = reinterpret_cast<k3d::uint8_t*>(boost::algorithm::find_first(stl.header, "COLOR=").end());
					base_color = k3d::color(static_cast<k3d::double_t>(color[0]/255.), static_cast<k3d::double_t>(color[1]/255.), static_cast<k3d::double_t>(color[2]/255.));
					is_magics = true;
				} else if(boost::algorithm::contains(stl.header, "MATERIAL="))
				{
					k3d::uint8_t* color = reinterpret_cast<k3d::uint8_t*>(boost::algorithm::find_first(stl.header, "MATERIAL=").end());
					base_color = k3d::color(static_cast<k3d::double_t>(color[0]/255.), static_cast<k3d::double_t>(color[1]/255.), static_cast<k3d::double_t>(color[2]/255.));
					is_magics = true;
				}
				const k3d::uint_t nfacets = stl.facets.size();
				vertex_counts.reserve(nfacets);
				face_normals.reserve(nfacets);
				face_colors.reserve(nfacets);
				vertex_indices.reserve(3*nfacets);
				for(k3d::uint_t f = 0; f != nfacets; ++f)
				{
					vertex_counts.push_back(3);
					k3d::point3 p0(stl.facets[f].v0[0], stl.facets[f].v0[1], stl.facets[f].v0[2]);
					k3d::point3 p1(stl.facets[f].v1[0], stl.facets[f].v1[1], stl.facets[f].v1[2]);
					k3d::point3 p2(stl.facets[f].v2[0], stl.facets[f].v2[1], stl.facets[f].v2[2]);
					vertex_indices.push_back(detail::add_point(point_map, p0));
					vertex_indices.push_back(detail::add_point(point_map, p1));
					vertex_indices.push_back(detail::add_point(point_map, p2));
					face_normals.push_back(k3d::normal3(stl.facets[f].normal[0], stl.facets[f].normal[1], stl.facets[f].normal[2]));
					face_colors.push_back(is_magics ? detail::convert_color_magics(stl.facets[f].color, base_color) : detail::convert_color_viscam(stl.facets[f].color, base_color));
				}
				detail::fill_points(point_map, points);
				k3d::polyhedron::primitive* polyhedron = k3d::polyhedron::create(Output, points, vertex_counts, vertex_indices, static_cast<k3d::imaterial*>(0));
				polyhedron->face_attributes.create(m_color_array.pipeline_value(), new k3d::mesh::colors_t(face_colors));
			}
		}
		catch(std::runtime_error& E)
		{
			k3d::log() << error << "STLMeshReader: " << E.what() << std::endl;
		}
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<mesh_reader,
                k3d::interface_list<k3d::imesh_source,
                k3d::interface_list<k3d::imesh_storage> > > factory(
			k3d::uuid(0x6518a3a6, 0x8147c354, 0x82bbc381, 0x82077bf8),
			"STLMeshReader",
			_("Mesh reader that loads external Wavefront (.stl) files into the document by reference"),
			"MeshReader");

		return factory;
	}

private:
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_threshold;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_store_normals;
	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_color_array;
};

k3d::iplugin_factory& mesh_reader_factory()
{
	return mesh_reader::get_factory();
}

} // namespace io

} // namespace stl

} // namespace module

