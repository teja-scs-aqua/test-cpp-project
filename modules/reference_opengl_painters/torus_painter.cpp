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
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/mesh.h>
#include <k3dsdk/mesh_painter_gl.h>
#include <k3dsdk/nurbs_curve.h>
#include <k3dsdk/painter_render_state_gl.h>
#include <k3dsdk/painter_selection_state_gl.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/torus.h>
#include <k3dsdk/utility_gl.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace opengl
{

namespace painters
{

/////////////////////////////////////////////////////////////////////////////
// torus_painter

class torus_painter :
	public k3d::gl::mesh_painter
{
	typedef k3d::gl::mesh_painter base;

public:
	torus_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document)
	{
	}

	void on_paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		const k3d::color color = RenderState.node_selection ? k3d::color(1, 1, 1) : k3d::color(0.8, 0.8, 0.8);
		const k3d::color selected_color = RenderState.show_component_selection ? k3d::color(1, 0, 0) : color;

		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::torus::const_primitive> torus(k3d::torus::validate(Mesh, **primitive));
			if(!torus)
				continue;

			glPolygonOffset(1.0, 1.0);
			glEnable(GL_POLYGON_OFFSET_FILL);
			glEnable(GL_LIGHTING);

			glMatrixMode(GL_MODELVIEW);
			for(k3d::uint_t i = 0; i != torus->matrices.size(); ++i)
			{
				k3d::gl::material(GL_FRONT_AND_BACK, GL_DIFFUSE, torus->selections[i] ? selected_color : color);

				glPushMatrix();
				k3d::gl::push_matrix(torus->matrices[i]);
				draw_solid(RenderState, torus->major_radii[i], torus->minor_radii[i], torus->phi_min[i], torus->phi_max[i], torus->sweep_angles[i]);
				glPopMatrix();
			}
		}
	}

	void on_select_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, const k3d::gl::painter_selection_state& SelectionState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		if(!SelectionState.select_component.count(k3d::selection::SURFACE))
			return;

		k3d::uint_t primitive_index = 0;
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive, ++primitive_index)
		{
			boost::scoped_ptr<k3d::torus::const_primitive> torus(k3d::torus::validate(Mesh, **primitive));
			if(!torus)
				continue;

			k3d::gl::push_selection_token(k3d::selection::PRIMITIVE, primitive_index);

			glDisable(GL_LIGHTING);

			glMatrixMode(GL_MODELVIEW);
			for(k3d::uint_t i = 0; i != torus->matrices.size(); ++i)
			{
				k3d::gl::push_selection_token(k3d::selection::SURFACE, i);

				glPushMatrix();
				k3d::gl::push_matrix(torus->matrices[i]);
				draw_solid(RenderState, torus->major_radii[i], torus->minor_radii[i], torus->phi_min[i], torus->phi_max[i], torus->sweep_angles[i]);
				glPopMatrix();

				k3d::gl::pop_selection_token(); // SURFACE
			}

			k3d::gl::pop_selection_token(); // PRIMITIVE
		}
	}

	void draw_solid(const k3d::gl::render_state& State, const k3d::double_t MajorRadius, const k3d::double_t MinorRadius, const k3d::double_t PhiMin, const k3d::double_t PhiMax, const k3d::double_t SweepAngle)
	{
		if(!MajorRadius)
			return;
		if(!MinorRadius)
			return;

		k3d::mesh::knots_t v_knots;
		k3d::mesh::weights_t v_weights;
		k3d::mesh::points_t v_arc_points;
		k3d::nurbs_curve::circular_arc(k3d::vector3(0, 1, 0), k3d::vector3(0, 0, 1), PhiMin, PhiMax, 4, v_knots, v_weights, v_arc_points);

		k3d::mesh::knots_t u_knots;
		k3d::mesh::weights_t u_weights;
		k3d::mesh::points_t u_arc_points;
		k3d::nurbs_curve::circular_arc(k3d::vector3(1, 0, 0), k3d::vector3(0, 1, 0), 0, SweepAngle, 4, u_knots, u_weights, u_arc_points);

		std::vector<GLfloat> gl_u_knot_vector(u_knots.begin(), u_knots.end());
		std::vector<GLfloat> gl_v_knot_vector(v_knots.begin(), v_knots.end());
		std::vector<GLfloat> gl_control_points;

		for(k3d::uint_t v = 0; v != v_arc_points.size(); ++v)
		{
			const k3d::point3 offset = MinorRadius * v_arc_points[v][2] * k3d::point3(0, 0, 1);
			const k3d::double_t radius2 = MajorRadius + (MinorRadius * v_arc_points[v][1]);
			const k3d::double_t v_weight = v_weights[v];

			for(k3d::uint_t u = 0; u != u_arc_points.size(); ++u)
			{
				gl_control_points.push_back(v_weight * u_weights[u] * (radius2 * u_arc_points[u][0] + offset[0]));
				gl_control_points.push_back(v_weight * u_weights[u] * (radius2 * u_arc_points[u][1] + offset[1]));
				gl_control_points.push_back(v_weight * u_weights[u] * (radius2 * u_arc_points[u][2] + offset[2]));
				gl_control_points.push_back(v_weight * u_weights[u]);
			}
		}

		GLUnurbsObj* const nurbs_renderer = gluNewNurbsRenderer();

		// Important!  We load our own matrices for efficiency (saves round-trips to the server) and to prevent problems with selection
		gluNurbsProperty(nurbs_renderer, GLU_AUTO_LOAD_MATRIX, GL_FALSE);
		gluNurbsProperty(nurbs_renderer, GLU_CULLING, GL_TRUE);
		GLfloat gl_modelview_matrix[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, gl_modelview_matrix);
		gluLoadSamplingMatrices(nurbs_renderer, gl_modelview_matrix, State.gl_projection_matrix, State.gl_viewport);

		gluBeginSurface(nurbs_renderer);
		gluNurbsSurface(nurbs_renderer, gl_u_knot_vector.size(), &gl_u_knot_vector[0], gl_v_knot_vector.size(), &gl_v_knot_vector[0], 4, 36, &gl_control_points[0], 3, 3, GL_MAP2_VERTEX_4);
		gluEndSurface(nurbs_renderer);

		gluDeleteNurbsRenderer(nurbs_renderer);
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<torus_painter, k3d::interface_list<k3d::gl::imesh_painter> > factory(
			k3d::uuid(0x271118dd, 0x2a47903b, 0xf36b96b8, 0xf6916c27),
			"OpenGLTorusPainter",
			_("Renders torus primitives using OpenGL"),
			"OpenGL Painter",
			k3d::iplugin_factory::STABLE);

		return factory;
	}
};

/////////////////////////////////////////////////////////////////////////////
// torus_painter_factory

k3d::iplugin_factory& torus_painter_factory()
{
	return torus_painter::get_factory();
}

} // namespace painters

} // namespace opengl

} // namespace module

