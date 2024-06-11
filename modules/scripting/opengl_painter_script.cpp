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
#include <k3dsdk/algebra.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/imesh_painter_gl.h>
#include <k3dsdk/mesh.h>
#include <k3dsdk/node.h>
#include <k3dsdk/resource/resource.h>
#include <k3dsdk/scripted_node.h>
#include <k3dsdk/selection.h>

namespace module
{

namespace scripting
{

/////////////////////////////////////////////////////////////////////////////
// opengl_painter_script

class opengl_painter_script :
	public k3d::scripted_node<k3d::node >,
	public k3d::gl::imesh_painter
{
	typedef k3d::scripted_node<k3d::node > base;

public:
	opengl_painter_script(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document)
	{
		set_script(k3d::resource::get_string("/module/scripting/opengl_painter_script.py"));
	}

	void paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		k3d::iscript_engine::context context;
		context["document"] = &document();
		context["node"] = static_cast<k3d::inode*>(this);
		context["mesh"] = &Mesh;
		context["paint"] = true;
		context["select"] = false;
		execute_script(context);
	}

	void select_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, const k3d::gl::painter_selection_state& SelectionState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		k3d::gl::push_selection_token(this);

		k3d::iscript_engine::context context;
		context["document"] = &document();
		context["node"] = static_cast<k3d::inode*>(this);
		context["mesh"] = &Mesh;
		context["paint"] = false;
		context["select"] = true;
		execute_script(context);

		k3d::gl::pop_selection_token();
	}

	void mesh_changed(const k3d::mesh& Mesh, k3d::ihint* Hint)
	{
	}

	k3d::iplugin_factory& factory()
	{
		return get_factory();
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<opengl_painter_script,
			k3d::interface_list<k3d::gl::imesh_painter > > factory(
			k3d::uuid(0xd841c417, 0x889a44d7, 0x8fe520eb, 0xf26dc650),
			"OpenGLPainterScript",
			_("Scripted Mesh Painter"),
			"OpenGL Painter Script",
			k3d::iplugin_factory::STABLE);

		return factory;
	}
};

/////////////////////////////////////////////////////////////////////////////
// opengl_painter_script_factory

k3d::iplugin_factory& opengl_painter_script_factory()
{
	return opengl_painter_script::get_factory();
}

} // namespace scripting

} // namespace module


