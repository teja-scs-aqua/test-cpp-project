// K-3D
// Copyright (c) 1995-2005, Timothy M. Shead
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

/** \file Painter that triangulates its input first
 * 	\author Bart Janssens (bart.janssens@lid.kviv.be)
 */

#include "cached_triangulation.h"
#include "colored_selection_painter_gl.h"
#include "utility.h"

#include <k3d-i18n-config.h>
#include <k3dsdk/array.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/gl/extension.h>
#include <k3dsdk/gl.h>
#include <k3dsdk/hints.h>
#include <k3dsdk/imesh_painter_gl.h>
#include <k3dsdk/node.h>
#include <k3dsdk/painter_render_state_gl.h>
#include <k3dsdk/painter_selection_state_gl.h>
#include <k3dsdk/polyhedron.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/utility_gl.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace opengl
{

namespace painters
{

/////////////////////////////////////////////////////////////////////////////
// hidden_line_painter

class hidden_line_painter :
	public colored_selection_painter
{
	typedef colored_selection_painter base;

public:
	hidden_line_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_line_width(init_owner(*this) + init_name("line_width") + init_label(_("Line Width")) + init_description(_("Control the rendered edge width (in pixels).")) + init_value(1.0) + init_constraint(k3d::data::constraint::minimum(0.0)) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::scalar)))
	{
		m_line_width.changed_signal().connect(make_async_redraw_slot());
	}

	void on_paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{		
		k3d::gl::store_attributes attributes;

		// Draw solid polygons for masking ...
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glColorMask(false, false, false, false);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(2.0, 1.0);
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		const color_t color = RenderState.node_selection ? selected_mesh_color() : unselected_mesh_color(RenderState.parent_selection);
		const color_t selected_color = RenderState.show_component_selection ? selected_component_color() : color;
		
		cached_triangulation& triangles = get_data<cached_triangulation>(&Mesh, this);
		const k3d::mesh::indices_t& face_starts = triangles.face_starts();
		if(face_starts.empty())
			return;

		const k3d::mesh::points_t& points = triangles.points();
		const cached_triangulation::indices_t& indices = triangles.indices();
		
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Mesh, **primitive));
			if(!polyhedron.get() || k3d::polyhedron::is_sds(*polyhedron))
				continue;
			
			glBegin(GL_TRIANGLES);
			for (k3d::uint_t face = 0; face != face_starts.size(); ++face)
			{
				k3d::uint_t startindex = face_starts[face];
				k3d::uint_t endindex = face+1 == (face_starts.size()) ? indices.size() : face_starts[face+1];
				for(k3d::uint_t corner = startindex; corner != endindex; ++corner)
					k3d::gl::vertex3d(points[indices[corner]]);
			}
			glEnd();
	
			// Draw wireframe edges ...
			glDepthMask(GL_FALSE);
			glColorMask(true, true, true, true);
			glDisable(GL_POLYGON_OFFSET_FILL);
	
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_LINE_SMOOTH);
			glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	
			glLineWidth(m_line_width.pipeline_value());
	
			glBegin(GL_LINES);
			const k3d::uint_t edge_count = polyhedron->edge_points.size();
			for(k3d::uint_t edge = 0; edge != edge_count; ++edge)
			{
				color4d(polyhedron->edge_selections[edge] ? selected_color : color);
				k3d::gl::vertex3d(points[polyhedron->edge_points[edge]]);
				k3d::gl::vertex3d(points[polyhedron->edge_points[polyhedron->clockwise_edges[edge]]]);
			}
			glEnd();
		}
	}
	
	void on_select_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, const k3d::gl::painter_selection_state& SelectionState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		if(!SelectionState.select_component.count(k3d::selection::UNIFORM))
			return;
			
		k3d::gl::store_attributes attributes;
		
		glDisable(GL_LIGHTING);

		glFrontFace(RenderState.inside_out ? GL_CCW : GL_CW);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		k3d::gl::set(GL_CULL_FACE, !SelectionState.select_backfacing);

		cached_triangulation& triangles = get_data<cached_triangulation>(&Mesh, this); 
		
		const k3d::mesh::indices_t& face_starts = triangles.face_starts();
		if (face_starts.empty())
			return;
		const k3d::mesh::points_t& points = triangles.points();
		const cached_triangulation::indices_t& indices = triangles.indices();

		k3d::uint_t face_offset = 0;
		k3d::uint_t primitive_index = 0;
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive, ++primitive_index)
		{
			boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Mesh, **primitive));
			if(!polyhedron.get() || k3d::polyhedron::is_sds(*polyhedron))
				continue;

			k3d::gl::push_selection_token(k3d::selection::PRIMITIVE, primitive_index);

			k3d::uint_t face_count = polyhedron->face_first_loops.size();
			for(k3d::uint_t poly_face = 0; poly_face != face_count; ++poly_face)
			{
				const k3d::uint_t face = poly_face + face_offset;
				k3d::gl::push_selection_token(k3d::selection::UNIFORM, face);
	
				k3d::uint_t startindex = face_starts[face];
				k3d::uint_t endindex = face+1 == (face_starts.size()) ? indices.size() : face_starts[face+1];
				glBegin(GL_TRIANGLES);
				for (k3d::uint_t corner = startindex; corner != endindex; ++corner)
					k3d::gl::vertex3d(points[indices[corner]]);
				glEnd();
	
				k3d::gl::pop_selection_token(); // UNIFORM
			}
			face_offset += face_count;

			k3d::gl::pop_selection_token(); // PRIMITIVE
		}
	}
	
	void on_mesh_changed(const k3d::mesh& Mesh, k3d::ihint* Hint)
	{
		if(!has_non_sds_polyhedra(Mesh))
			return;
		
		schedule_data<cached_triangulation>(&Mesh, Hint, this);
	}
	
	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<hidden_line_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
			k3d::uuid(0x5c0a98d1, 0x4148e5e1, 0x901e36b5, 0xc5aa6ee6),
			"OpenGLHiddenLinePainter",
			_("Renders meshes using a classic hidden-line effect."),
			"OpenGL Painter",
			k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}

private:
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, writable_property, with_serialization) m_line_width;
};

/////////////////////////////////////////////////////////////////////////////
// hidden_line_painter_factory

k3d::iplugin_factory& hidden_line_painter_factory()
{
	return hidden_line_painter::get_factory();
}

} // namespace opengl

} // namespace painters

} // namespace module

