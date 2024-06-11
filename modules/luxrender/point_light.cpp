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
	\author Tim Shead <tshead@k-3d.com>
*/

#include "light.h"
#include "utility.h"

#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/node.h>
#include <k3dsdk/renderable_gl.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/transformable.h>
#include <k3dsdk/transform.h>

namespace module
{

namespace luxrender
{

/////////////////////////////////////////////////////////////////////////////
// point_light

class point_light :
	public k3d::gl::renderable<k3d::transformable<light> >
{
	typedef k3d::gl::renderable<k3d::transformable<light> > base;

public:
	point_light(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_quadric(gluNewQuadric()),
		m_color(init_owner(*this) + init_name("L") + init_label(_("Color")) + init_description(_("Color")) + init_value(k3d::color(0.8, 0.8, 0.8)))
	{
		m_input_matrix.changed_signal().connect(make_async_redraw_slot());
	}

	~point_light()
	{
		gluDeleteQuadric(m_quadric);
	}

	void on_setup(std::ostream& Stream)
	{
		Stream << k3d::standard_indent << "LightSource \"point\"\n" << k3d::push_indent;
		Stream << k3d::standard_indent << "\"point from\" [" << convert(k3d::world_position(*this)) << "]\n";
		Stream << k3d::standard_indent << "\"color L\" [" << m_color.pipeline_value() << "]\n";
		Stream << k3d::pop_indent;
	}

	void on_gl_draw(const k3d::gl::render_state& State)
	{
		k3d::gl::store_attributes attributes;

		k3d::gl::material(GL_FRONT_AND_BACK, GL_AMBIENT, k3d::color(0, 0, 0));
		k3d::gl::material(GL_FRONT_AND_BACK, GL_DIFFUSE, State.node_selection ? k3d::color(1, 1, 1) : k3d::color(0.8, 0.8, 0.2));
		k3d::gl::material(GL_FRONT_AND_BACK, GL_SPECULAR, k3d::color(0, 0, 0));
		k3d::gl::material(GL_FRONT_AND_BACK, GL_EMISSION, k3d::color(0.5, 0.5, 0));

		glEnable(GL_LIGHTING);
		draw_geometry();
	}

	void on_gl_select(const k3d::gl::render_state& State, const k3d::gl::selection_state& SelectState)
	{
		k3d::gl::store_attributes attributes;

		glDisable(GL_LIGHTING);

		k3d::gl::push_selection_token(this);
		draw_geometry();
		k3d::gl::pop_selection_token();
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<point_light,
			k3d::interface_list<k3d::imatrix_source,
			k3d::interface_list<k3d::imatrix_sink > > > factory(
				k3d::uuid(0xcbc855c0, 0xed49e9d1, 0x37fc2a83, 0xc58e863f),
				"LuxRenderPointLight",
				_("LuxRender Point Light"),
				"LuxRender Light",
				k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}

private:
	void draw_geometry()
	{
		gluSphere(m_quadric, 0.5, 8, 8);
	}

	/// Stores a GLU quadric object for drawing the manipulators
	GLUquadricObj* const m_quadric;
	k3d_data(k3d::color, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_color;
};

k3d::iplugin_factory& point_light_factory()
{
	return point_light::get_factory();
}

} // namespace luxrender

} // namespace module


