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

/** \file
	\author Timothy M. Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>

#include <k3dsdk/basic_math.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/geometry.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/mesh_modifier.h>
#include <k3dsdk/mesh_selection_sink.h>
#include <k3dsdk/node.h>
#include <k3dsdk/polyhedron.h>

#include <boost/scoped_ptr.hpp>
#include <iterator>

namespace module
{

namespace mesh_attributes
{

/////////////////////////////////////////////////////////////////////////////
// calculate_normals

class calculate_normals :
	public k3d::mesh_selection_sink<k3d::mesh_modifier<k3d::node > >
{
	typedef k3d::mesh_selection_sink<k3d::mesh_modifier<k3d::node > > base;

public:
	calculate_normals(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_max_angle(init_owner(*this) + init_name("max_angle") + init_label(_("Maximum Angle")) + init_description(_("Normals will not be smoothed across points sharper than this angle (only applies to Vertex Normals).")) + init_value(k3d::radians(89.0)) + init_step_increment(k3d::radians(1.0)) + init_units(typeid(k3d::measurement::angle))),
		m_face(init_owner(*this) + init_name("face") + init_label(_("Face Normals")) + init_description(_("Generate per-face normals.")) + init_value(false)),
		m_vertex(init_owner(*this) + init_name("vertex") + init_label(_("Vertex Normals")) + init_description(_("Generate per-vertex normals.")) + init_value(true)),
		m_point(init_owner(*this) + init_name("point") + init_label(_("Point Normals")) + init_description(_("Generate per-point normals.")) + init_value(false)),
		m_face_array(init_owner(*this) + init_name("face_array") + init_label(_("Face Array Name")) + init_description(_("Face output array name.")) + init_value(k3d::string_t("N"))),
		m_vertex_array(init_owner(*this) + init_name("vertex_array") + init_label(_("Vertex Array Name")) + init_description(_("Vertex output array name.")) + init_value(k3d::string_t("N"))),
		m_point_array(init_owner(*this) + init_name("point_array") + init_label(_("Point Array Name")) + init_description(_("Point output array name.")) + init_value(k3d::string_t("N")))
	{
		m_mesh_selection.changed_signal().connect(make_update_mesh_slot());

		m_max_angle.changed_signal().connect(make_update_mesh_slot());

		m_face.changed_signal().connect(make_update_mesh_slot());
		m_vertex.changed_signal().connect(make_update_mesh_slot());
		m_point.changed_signal().connect(make_update_mesh_slot());
		
		m_face_array.changed_signal().connect(make_update_mesh_slot());
		m_vertex_array.changed_signal().connect(make_update_mesh_slot());
		m_point_array.changed_signal().connect(make_update_mesh_slot());
	}

	void on_create_mesh(const k3d::mesh& Input, k3d::mesh& Output)
	{
	}

	void on_update_mesh(const k3d::mesh& Input, k3d::mesh& Output)
	{
		Output = Input;

		k3d::geometry::selection::merge(m_mesh_selection.pipeline_value(), Output);

		if(!Output.points)
			return;

		const k3d::bool_t store_face = m_face.pipeline_value();
		const k3d::bool_t store_vertex = m_vertex.pipeline_value();
		const k3d::bool_t store_point = m_point.pipeline_value();
		const k3d::mesh::points_t& points = *Output.points;

		// Optionally store point normals ...
		k3d::mesh::normals_t* point_normals = 0;
		if(store_point)
			point_normals = &Output.point_attributes.create(m_point_array.pipeline_value(), new k3d::mesh::normals_t(points.size()));

		for(k3d::mesh::primitives_t::iterator primitive = Output.primitives.begin(); primitive != Output.primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::polyhedron::primitive> polyhedron(k3d::polyhedron::validate(Output, *primitive));
			if(!polyhedron)
				continue;

			const k3d::uint_t face_begin = 0;
			const k3d::uint_t face_end = face_begin + polyhedron->face_first_loops.size();

			// Compute per-face normals (used for all subsequent calculations) ...
			k3d::mesh::normals_t face_normals(polyhedron->face_first_loops.size());
			for(k3d::uint_t face = face_begin; face != face_end; ++face)
				face_normals[face] = k3d::normalize(k3d::polyhedron::normal(polyhedron->vertex_points, polyhedron->clockwise_edges, points, polyhedron->loop_first_edges[polyhedron->face_first_loops[face]]));

			// Optionally store the face normals ...
			if(store_face)
				polyhedron->face_attributes.create(m_face_array.pipeline_value(), new k3d::mesh::normals_t(face_normals));

			// Optionally compute per-vertex normals ...
			if(store_vertex)
			{
				const k3d::double_t cos_max_angle = std::cos(std::max(0.0, m_max_angle.pipeline_value()));

				k3d::mesh::normals_t& vertex_normals = polyhedron->vertex_attributes.create(m_vertex_array.pipeline_value(), new k3d::mesh::normals_t(polyhedron->vertex_points.size()));

				k3d::mesh::indices_t point_first_faces;
				k3d::mesh::counts_t point_face_counts;
				k3d::mesh::indices_t point_faces;
				k3d::polyhedron::create_point_face_lookup(polyhedron->face_first_loops, polyhedron->face_loop_counts, polyhedron->loop_first_edges, polyhedron->vertex_points, polyhedron->clockwise_edges, points, point_first_faces, point_face_counts, point_faces);

				for(k3d::uint_t face = face_begin; face != face_end; ++face)
				{
					const k3d::normal3 face_normal = face_normals[face];

					const k3d::uint_t loop_begin = polyhedron->face_first_loops[face];
					const k3d::uint_t loop_end = loop_begin + polyhedron->face_loop_counts[face];
					for(k3d::uint_t loop = loop_begin; loop != loop_end; ++loop)
					{
						const k3d::uint_t first_edge = polyhedron->loop_first_edges[loop];
						for(k3d::uint_t edge = first_edge; ;)
						{
							vertex_normals[edge] += face_normal;

							if(polyhedron->face_selections[face])
							{
								const k3d::uint_t point_face_begin = point_first_faces[polyhedron->vertex_points[edge]];
								const k3d::uint_t point_face_end = point_face_begin + point_face_counts[polyhedron->vertex_points[edge]];
								for(k3d::uint_t point_face = point_face_begin; point_face != point_face_end; ++point_face)
								{
									const k3d::uint_t adjacent_face = point_faces[point_face];
									if(adjacent_face == face)
										continue;

									if(!polyhedron->face_selections[adjacent_face])
										continue;

									const k3d::normal3 adjacent_normal = face_normals[adjacent_face];

									const k3d::double_t cos_angle = adjacent_normal * face_normal;
									if(cos_angle < cos_max_angle)
										continue;

									vertex_normals[edge] += adjacent_normal;
								}
							}

							edge = polyhedron->clockwise_edges[edge];
							if(edge == first_edge)
								break;
						}
					}
				}
      			}

			// Optionally compute per-point normals as the sum of adjacent face normals ...
			if(store_point)
			{
				for(k3d::uint_t face = face_begin; face != face_end; ++face)
				{
					const k3d::uint_t loop_begin = polyhedron->face_first_loops[face];
					const k3d::uint_t loop_end = loop_begin + polyhedron->face_loop_counts[face];
					for(k3d::uint_t loop = loop_begin; loop != loop_end; ++loop)
					{
						const k3d::uint_t first_edge = polyhedron->loop_first_edges[loop];
						for(k3d::uint_t edge = first_edge; ;)
						{
							(*point_normals)[polyhedron->vertex_points[edge]] += face_normals[face];

							edge = polyhedron->clockwise_edges[edge];
							if(edge == first_edge)
								break;
						}
					}
				}
			}
		}
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<calculate_normals,
			k3d::interface_list<k3d::imesh_source,
			k3d::interface_list<k3d::imesh_sink > > > factory(
				k3d::uuid(0xa6d565ee, 0x6b4a065d, 0x2430ca88, 0xb0bd88a1),
				"CalculateNormals",
				_("Calculates a variety of polygon normals"),
				"MeshAttributes",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_max_angle;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_face;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_vertex;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_point;
	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_face_array;
	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_vertex_array;
	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_point_array;
};

/////////////////////////////////////////////////////////////////////////////
// calculate_normals_factory

k3d::iplugin_factory& calculate_normals_factory()
{
	return calculate_normals::get_factory();
}

} // namespace mesh_attributes

} // namespace module

