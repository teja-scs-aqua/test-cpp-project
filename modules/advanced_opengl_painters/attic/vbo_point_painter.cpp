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

/** \file Paint points using a VBO
 */

#include "vbo_colored_selection_painter_gl.h"
//#include "normal_cache.h"
//#include "utility.h"
#include "painter_cache.h"
#include "vbo.h"

#include <k3d-i18n-config.h>
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
// vbo_point_painter

class vbo_point_painter :
	public vbo_colored_selection_painter
{
	typedef vbo_colored_selection_painter base;

public:
	vbo_point_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document)
	{
	}
		
	void on_paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		return_if_fail(k3d::gl::extension::query_vbo());

		if(!Mesh.points || Mesh.points->empty())
			return;

		const point_data& data = get_cached_data<point_vbo>(Mesh.points.get(), ChangedSignal).value(Mesh);
		bind_vertex_buffer(data.points);
		bind_texture_buffer(data.selection);

		glDisable(GL_LIGHTING);
		glDrawArrays(GL_POINTS, 0, Mesh.points->size());
	}
	
	void on_select_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, const k3d::gl::painter_selection_state& SelectionState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		return_if_fail(k3d::gl::extension::query_vbo());

		if(!Mesh.points || Mesh.points->empty())
			return;

		if (!SelectionState.select_component.count(k3d::selection::POINT))
			return;

		bind_vertex_buffer(get_cached_data<point_vbo>(Mesh.points.get(), ChangedSignal).value(Mesh).points);

		glDisable(GL_LIGHTING);

		const size_t point_count = Mesh.points->size();
		for(size_t point = 0; point != point_count; ++point)
		{
			//TODO: restore backfacing selection
//			if (!valid_polyhedra || SelectionState.select_backfacing ||
//					(!SelectionState.select_backfacing &&
//							!backfacing(Mesh.points->at(point) * RenderState.matrix,RenderState.camera, get_data<normal_cache>(&Mesh, this).point_normals(this).at(point))))
//			{
				k3d::gl::push_selection_token(k3d::selection::POINT, point);

				glBegin(GL_POINTS);
				glArrayElement(point);
				glEnd();

				k3d::gl::pop_selection_token(); // k3d::selection::POINT
			//}
		}
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<vbo_point_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
				k3d::uuid(0xe2495ce1, 0x0c4c42eb, 0x81142404, 0x3c25889e),
		"OpenGLVBOPointPainter",
		_("Renders mesh points (OpenGL VBOs)"),
		"OpenGL Painter",
		k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}
};

	/////////////////////////////////////////////////////////////////////////////
// vbo_point_painter_factory

	k3d::iplugin_factory& vbo_point_painter_factory()
	{
		return vbo_point_painter::get_factory();
	}

} // namespace painters

} // namespace opengl

} // namespace module


