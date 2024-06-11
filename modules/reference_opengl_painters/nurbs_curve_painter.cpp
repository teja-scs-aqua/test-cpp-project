// K-3D
// Copyright (c) 1995-2007, Timothy M. Shead
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
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/mesh_painter_gl.h>
#include <k3dsdk/nurbs_curve.h>
#include <k3dsdk/painter_render_state_gl.h>
#include <k3dsdk/painter_selection_state_gl.h>
#include <k3dsdk/selection.h>

#include <boost/scoped_ptr.hpp>

#if defined K3D_API_DARWIN
  #if K3D_OSX_MAJOR_VERSION == 10 & K3D_OSX_MINOR_VERSION > 4 // OSX >= 10.5
	  #define GLU_NURBS_CALLBACK(callback) (GLvoid(*)())callback
  #else // OSX < 10.5
	  #define GLU_NURBS_CALLBACK(callback) (GLvoid(*)(...))callback
  #endif
#elif defined K3D_API_WIN32
	#define GLU_NURBS_CALLBACK(callback) (_GLUfuncptr)callback
#else
	#define GLU_NURBS_CALLBACK(callback) (void(*)())callback
#endif // !K3D_API_DARWIN

namespace module
{

namespace opengl
{

namespace painters
{

static void on_nurbs_error(GLenum ErrorCode)
{
	k3d::log() << error << "NURBS curve error: " << gluErrorString(ErrorCode) << std::endl;
}

/////////////////////////////////////////////////////////////////////////////
// nurbs_curve_painter

class nurbs_curve_painter :
	public k3d::gl::mesh_painter
{
	typedef k3d::gl::mesh_painter base;

public:
	nurbs_curve_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		nurbs_renderer(0)
	{
	}

	~nurbs_curve_painter()
	{
		if(nurbs_renderer)
                        gluDeleteNurbsRenderer(nurbs_renderer);
	}

	void initialize()
	{
		if(nurbs_renderer)
			return;

		nurbs_renderer = gluNewNurbsRenderer();

		// Important!  We load our own matrices for efficiency (saves round-trips to the server) and to prevent problems with selection
		gluNurbsProperty(nurbs_renderer, GLU_AUTO_LOAD_MATRIX, GL_FALSE);
		gluNurbsProperty(nurbs_renderer, GLU_CULLING, GL_FALSE);
		gluNurbsCallback(nurbs_renderer, GLU_ERROR, GLU_NURBS_CALLBACK(on_nurbs_error));
	}

	void on_paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::nurbs_curve::const_primitive> nurbs_curve(k3d::nurbs_curve::validate(Mesh, **primitive));
			if(!nurbs_curve)
				continue;

			initialize();

			const k3d::mesh::points_t& points = *Mesh.points;

			k3d::gl::store_attributes attributes;
			glDisable(GL_LIGHTING);

			GLfloat gl_modelview_matrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, gl_modelview_matrix);
			gluLoadSamplingMatrices(nurbs_renderer, gl_modelview_matrix, RenderState.gl_projection_matrix, RenderState.gl_viewport);

			const k3d::color color = RenderState.node_selection ? k3d::color(1, 1, 1) : k3d::color(0, 0, 0);
			const k3d::color selected_color = RenderState.show_component_selection ? k3d::color(1, 0, 0) : color;

			const k3d::uint_t curve_begin = 0;
			const k3d::uint_t curve_end = curve_begin + nurbs_curve->curve_first_points.size();
			for(k3d::uint_t curve = curve_begin; curve != curve_end; ++curve)
			{
				const k3d::uint_t curve_point_count = nurbs_curve->curve_point_counts[curve];
				const k3d::uint_t curve_point_begin = nurbs_curve->curve_first_points[curve];
				const k3d::uint_t curve_point_end = curve_point_begin + curve_point_count;
				const k3d::uint_t curve_order = nurbs_curve->curve_orders[curve];
				const k3d::uint_t curve_first_knot = nurbs_curve->curve_first_knots[curve];

				k3d::gl::color3d(nurbs_curve->curve_selections[curve] ? selected_color : color);

				std::vector<GLfloat> gl_knot_vector(&nurbs_curve->curve_knots[curve_first_knot], &nurbs_curve->curve_knots[curve_first_knot + curve_point_count + curve_order]);

				std::vector<GLfloat> gl_control_points;
				gl_control_points.reserve(4 * curve_point_count);
				for(k3d::uint_t curve_point = curve_point_begin; curve_point != curve_point_end; ++curve_point)
				{
					const k3d::double_t curve_point_weight = nurbs_curve->curve_point_weights[curve_point];
					const k3d::point3 weighted_curve_point = curve_point_weight * points[nurbs_curve->curve_points[curve_point]];

					gl_control_points.push_back(static_cast<GLfloat>(weighted_curve_point[0]));
					gl_control_points.push_back(static_cast<GLfloat>(weighted_curve_point[1]));
					gl_control_points.push_back(static_cast<GLfloat>(weighted_curve_point[2]));
					gl_control_points.push_back(static_cast<GLfloat>(curve_point_weight));
				}

				gluBeginCurve(nurbs_renderer);
				gluNurbsCurve(nurbs_renderer, gl_knot_vector.size(), &gl_knot_vector[0], 4, &gl_control_points[0], curve_order, GL_MAP1_VERTEX_4);
				gluEndCurve(nurbs_renderer);
			}
		}
	}

	void on_select_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, const k3d::gl::painter_selection_state& SelectionState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		if(!SelectionState.select_component.count(k3d::selection::CURVE))
			return;

		k3d::uint_t primitive_index = 0;
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive, ++primitive_index)
		{
			boost::scoped_ptr<k3d::nurbs_curve::const_primitive> nurbs_curve(k3d::nurbs_curve::validate(Mesh, **primitive));
			if(!nurbs_curve)
				continue;

			initialize();

			const k3d::mesh::points_t& points = *Mesh.points;

			k3d::gl::store_attributes attributes;
			glDisable(GL_LIGHTING);

			k3d::gl::push_selection_token(k3d::selection::PRIMITIVE, primitive_index);

			GLfloat gl_modelview_matrix[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, gl_modelview_matrix);
			gluLoadSamplingMatrices(nurbs_renderer, gl_modelview_matrix, RenderState.gl_projection_matrix, RenderState.gl_viewport);

			const k3d::uint_t curve_begin = 0;
			const k3d::uint_t curve_end = curve_begin + nurbs_curve->curve_first_points.size();
			k3d::uint_t curve_index = 0;
			for(k3d::uint_t curve = curve_begin; curve != curve_end; ++curve, ++curve_index)
			{
				k3d::gl::push_selection_token(k3d::selection::CURVE, curve_index);

				const k3d::uint_t curve_point_count = nurbs_curve->curve_point_counts[curve];
				const k3d::uint_t curve_point_begin = nurbs_curve->curve_first_points[curve];
				const k3d::uint_t curve_point_end = curve_point_begin + curve_point_count;
				const k3d::uint_t curve_order = nurbs_curve->curve_orders[curve];
				const k3d::uint_t curve_first_knot = nurbs_curve->curve_first_knots[curve];

				std::vector<GLfloat> gl_knot_vector(&nurbs_curve->curve_knots[curve_first_knot], &nurbs_curve->curve_knots[curve_first_knot + curve_point_count + curve_order]);

				std::vector<GLfloat> gl_control_points;
				gl_control_points.reserve(4 * curve_point_count);
				for(k3d::uint_t curve_point = curve_point_begin; curve_point != curve_point_end; ++curve_point)
				{
					const k3d::double_t curve_point_weight = nurbs_curve->curve_point_weights[curve_point];
					const k3d::point3 weighted_curve_point = curve_point_weight * points[nurbs_curve->curve_points[curve_point]];

					gl_control_points.push_back(static_cast<GLfloat>(weighted_curve_point[0]));
					gl_control_points.push_back(static_cast<GLfloat>(weighted_curve_point[1]));
					gl_control_points.push_back(static_cast<GLfloat>(weighted_curve_point[2]));
					gl_control_points.push_back(static_cast<GLfloat>(curve_point_weight));
				}

				gluBeginCurve(nurbs_renderer);
				gluNurbsCurve(nurbs_renderer, gl_knot_vector.size(), &gl_knot_vector[0], 4, &gl_control_points[0], curve_order, GL_MAP1_VERTEX_4);
				gluEndCurve(nurbs_renderer);

				k3d::gl::pop_selection_token(); // CURVE
			}

			k3d::gl::pop_selection_token(); // PRIMITIVE
		}
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<nurbs_curve_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
			k3d::uuid(0x86705b44, 0xef534dc4, 0x86d9fc7a, 0x8709ec59),
			"OpenGLNURBSCurvePainter",
			_("Renders NURBS curves"),
			"OpenGL Painter",
			k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	GLUnurbsObj* nurbs_renderer;
};

/////////////////////////////////////////////////////////////////////////////
// nurbs_curve_painter_factory

k3d::iplugin_factory& nurbs_curve_painter_factory()
{
	return nurbs_curve_painter::get_factory();
}

} // namespace painters

} // namespace opengl

} // namespace module


