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
*/

#include "colored_selection_painter_gl.h"

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
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

/////////////////////////////////////////////////////////////////////////////
// edge_painter

class edge_painter :
	public colored_selection_painter
{
	typedef colored_selection_painter base;
public:
	edge_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document)
	{
	}

	void on_paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Mesh, **primitive));
			if(!polyhedron.get())
				continue;
		
			const k3d::mesh::points_t& points = *Mesh.points;
		
			k3d::gl::store_attributes attributes;
			glDisable(GL_LIGHTING);
	
			const k3d::color color = RenderState.node_selection ? selected_mesh_color() : unselected_mesh_color(RenderState.parent_selection);
			const k3d::color selected_color = RenderState.show_component_selection ? selected_component_color() : color;
			const k3d::uint_t edge_count = polyhedron->clockwise_edges.size();
			
			glBegin(GL_LINES);

			// Render selected split-edges before unselected, so the selection is always visible
			k3d::gl::color3d(selected_color);
			for(k3d::uint_t edge = 0; edge != edge_count; ++edge)
			{
				if(!polyhedron->edge_selections[edge])
					continue;
				k3d::gl::vertex3d(points[polyhedron->vertex_points[edge]]);
				k3d::gl::vertex3d(points[polyhedron->vertex_points[polyhedron->clockwise_edges[edge]]]);
			}

			k3d::gl::color3d(color);
			for(k3d::uint_t edge = 0; edge != edge_count; ++edge)
			{
				if(polyhedron->edge_selections[edge])
					continue;
				k3d::gl::vertex3d(points[polyhedron->vertex_points[edge]]);
				k3d::gl::vertex3d(points[polyhedron->vertex_points[polyhedron->clockwise_edges[edge]]]);
			}

			glEnd();
		}
	}
	
	void on_select_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, const k3d::gl::painter_selection_state& SelectionState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		if(!SelectionState.select_component.count(k3d::selection::EDGE))
			return;

		k3d::uint_t primitive_index = 0;
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive, ++primitive_index)
		{
			boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Mesh, **primitive));
			if(!polyhedron.get())
				continue;
		
			k3d::gl::push_selection_token(k3d::selection::PRIMITIVE, primitive_index);

			const k3d::mesh::points_t& points = *Mesh.points;

			k3d::gl::store_attributes attributes;
			glDisable(GL_LIGHTING);
	
			const k3d::uint_t edge_count = polyhedron->clockwise_edges.size();
			for(k3d::uint_t edge = 0; edge != edge_count; ++edge)
			{
				k3d::gl::push_selection_token(k3d::selection::EDGE, edge);

				glBegin(GL_LINES);
				k3d::gl::vertex3d(points[polyhedron->vertex_points[edge]]);
				k3d::gl::vertex3d(points[polyhedron->vertex_points[polyhedron->clockwise_edges[edge]]]);
				glEnd();

				k3d::gl::pop_selection_token(); // EDGE
			}

			k3d::gl::pop_selection_token(); // PRIMITIVE
		}
	}
	
	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<edge_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
			k3d::uuid(0xb1260f93, 0xe16e4ab2, 0xbd6a7cbd, 0x85ddca8b),
			"OpenGLEdgePainter",
			_("Renders mesh edges (OpenGL 1.1)"),
			"OpenGL Painter",
			k3d::iplugin_factory::STABLE);

		return factory;
	}
};

/////////////////////////////////////////////////////////////////////////////
// edge_painter_factory

k3d::iplugin_factory& edge_painter_factory()
{
	return edge_painter::get_factory();
}

} // namespace painters

} // namespace opengl

} // namespace module


