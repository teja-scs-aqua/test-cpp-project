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
	\author Bart Janssens (bart.janssens@lid.kviv.be)
*/

#include "colored_selection_painter_gl.h"
#include "normal_cache.h"

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/hints.h>
#include <k3dsdk/mesh_painter_gl.h>
#include <k3dsdk/painter_render_state_gl.h>
#include <k3dsdk/painter_selection_state_gl.h>
#include <k3dsdk/polyhedron.h>
#include <k3dsdk/selection.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace opengl
{

namespace painters
{

namespace detail
{

/// Returns a point offset by Offset * edge_length into the face, normal to the edge vector and inside the face plane
const k3d::point3 get_offset_point(const k3d::mesh::indices_t& EdgePoints, const k3d::mesh::indices_t& ClockwiseEdges, const k3d::mesh::points_t& Points, const k3d::uint_t EdgeIndex, const k3d::double_t Offset)
{
	k3d::point3 corner = Points[EdgePoints[EdgeIndex]];
	k3d::point3 next_corner = Points[EdgePoints[ClockwiseEdges[EdgeIndex]]];
	k3d::normal3 normal = k3d::normalize(k3d::polyhedron::normal(EdgePoints, ClockwiseEdges, Points, EdgeIndex));
	k3d::vector3 edge_vector = next_corner - corner;
	k3d::vector3 inward_vector = normal ^ edge_vector;
	return corner + Offset*inward_vector + Offset*edge_vector;
}

} // namespace detail

/////////////////////////////////////////////////////////////////////////////
// face_varying_painter

class face_varying_painter :
	public colored_selection_painter
{
	typedef colored_selection_painter base;

public:
	face_varying_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_point_size(init_owner(*this) + init_name("point_size") + init_label(_("Point Size")) + init_description(_("Size for the points")) + init_value(6.0) + init_constraint(k3d::data::constraint::minimum(1.0, k3d::data::constraint::maximum<double>(50.0))) + init_step_increment(1.0) + init_units(typeid(k3d::measurement::scalar))),
		m_offset(init_owner(*this) + init_name("offset") + init_label(_("Offset")) + init_description(_("Offset factor for the point, in units of edge length")) + init_value(0.1) + init_constraint(k3d::data::constraint::minimum(0.0, k3d::data::constraint::maximum<double>(1.0))) + init_step_increment(0.05) + init_units(typeid(k3d::measurement::scalar)))
	{
		m_point_size.changed_signal().connect(make_async_redraw_slot());
		m_offset.changed_signal().connect(make_async_redraw_slot());
	}

	void on_paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		const k3d::mesh::points_t& points = *Mesh.points;
		const k3d::mesh::selection_t& point_selection = *Mesh.point_selection;

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_DOUBLE, 0, &points[0]);

		k3d::gl::store_attributes attributes;
		glDisable(GL_LIGHTING);

		const color_t color = RenderState.node_selection ? selected_mesh_color() : unselected_mesh_color(RenderState.parent_selection);
		const color_t selected_color = RenderState.show_component_selection ? selected_component_color() : color;

		enable_blending();

		GLfloat old_point_size;
		glGetFloatv(GL_POINT_SIZE, &old_point_size);
		glPointSize(m_point_size.pipeline_value());
		const k3d::double_t offset = m_offset.pipeline_value();

		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Mesh, **primitive));
			if(!polyhedron.get())
				continue;
			glBegin(GL_POINTS);
			const k3d::uint_t face_begin = 0;
			const k3d::uint_t face_end = polyhedron->face_first_loops.size();
			for(k3d::uint_t face = face_begin; face != face_end; ++face)
			{
				const k3d::uint_t loop_begin = polyhedron->face_first_loops[face];
				const k3d::uint_t loop_end = loop_begin +polyhedron->face_loop_counts[face];
				for(k3d::uint_t loop = 0; loop != loop_end; ++loop)
				{
					const k3d::uint_t first_edge = polyhedron->loop_first_edges[loop];
					for(k3d::uint_t edge = first_edge; ;)
					{
						color4d(polyhedron->edge_selections[edge] ? selected_color : color);
						k3d::gl::vertex3d(detail::get_offset_point(polyhedron->edge_points, polyhedron->clockwise_edges, points, edge, offset));
						edge = polyhedron->clockwise_edges[edge];
						if(edge == first_edge)
							break;
					}
				}
			}
			glEnd();
		}
		glPointSize(old_point_size);
		disable_blending();
	}

	void on_select_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, const k3d::gl::painter_selection_state& SelectionState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		if(!SelectionState.select_component.count(k3d::selection::SPLIT_EDGE))
			return;

		const k3d::mesh::points_t& points = *Mesh.points;

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_DOUBLE, 0, &points[0]);

		k3d::gl::store_attributes attributes;
		glDisable(GL_LIGHTING);

		GLfloat old_point_size;
		glGetFloatv(GL_POINT_SIZE, &old_point_size);
		glPointSize(m_point_size.pipeline_value());
		const k3d::double_t offset = m_offset.pipeline_value();

		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Mesh, **primitive));
			if(!polyhedron.get())
				continue;
			const k3d::uint_t face_begin = 0;
			const k3d::uint_t face_end = polyhedron->face_first_loops.size();
			for(k3d::uint_t face = face_begin; face != face_end; ++face)
			{
				const k3d::uint_t loop_begin = polyhedron->face_first_loops[face];
				const k3d::uint_t loop_end = loop_begin + polyhedron->face_loop_counts[face];
				for(k3d::uint_t loop = 0; loop != loop_end; ++loop)
				{
					const k3d::uint_t first_edge = polyhedron->loop_first_edges[loop];
					for(k3d::uint_t edge = first_edge; ;)
					{
						k3d::gl::push_selection_token(k3d::selection::SPLIT_EDGE, edge);
	
						glBegin(GL_POINTS);
						k3d::gl::vertex3d(detail::get_offset_point(polyhedron->edge_points, polyhedron->clockwise_edges, points, edge, offset));
						glEnd();
	
						k3d::gl::pop_selection_token(); // SPLIT_EDGE
	
						edge = polyhedron->clockwise_edges[edge];
						if(edge == first_edge)
							break;
					}
				}
			}
		}
		glPointSize(old_point_size);
	}

	void on_mesh_changed(const k3d::mesh& Mesh, k3d::ihint* Hint)
	{
		schedule_data<normal_cache>(&Mesh, Hint, this);
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<face_varying_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
			k3d::uuid(0xfbd81412, 0xa94df849, 0x1172a09e, 0xe08104f4),
			"OpenGLFaceVaryingPainter",
			_("Renders selection hooks for facevarying data"),
			"OpenGL Painter",
			k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}

private:
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_point_size;
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_offset;
};

/////////////////////////////////////////////////////////////////////////////
// face_varying_painter_factory

k3d::iplugin_factory& face_varying_painter_factory()
{
	return face_varying_painter::get_factory();
}

} // namespace painters

} // namespace opengl

} // namespace module


