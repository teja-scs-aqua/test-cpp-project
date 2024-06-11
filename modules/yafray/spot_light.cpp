// K-3D
// Copyright (c) 1995-2006, Timothy M. Shead
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
		\author Romain Behar <romainbehar@yahoo.com>
*/

#include "lights.h"

namespace module
{

namespace yafray
{

/////////////////////////////////////////////////////////////////////////////
// spot_light

class spot_light :
	public colored_light<light<k3d::gl::renderable<k3d::transformable<k3d::node > > > >
{
	typedef colored_light<light<k3d::gl::renderable<k3d::transformable<k3d::node > > > > base;

public:
	spot_light(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_quadric(gluNewQuadric()),
		m_power(init_owner(*this) + init_name("power") + init_label(_("power")) + init_description(_("Power")) + init_value(400.0)),
		m_size(init_owner(*this) + init_name("size") + init_label(_("size")) + init_description(_("Angle")) + init_value(k3d::radians(30.0))),
		m_nblend(init_owner(*this) + init_name("nblend") + init_label(_("nblend")) + init_description(_("Edge softness")) + init_value(5.0)),
		m_beam_falloff(init_owner(*this) + init_name("beam_falloff") + init_label(_("beam_falloff")) + init_description(_("Beam falloff")) + init_value(10.0))
	{
	}

	~spot_light()
	{
		gluDeleteQuadric(m_quadric);
	}

	void setup_light(const k3d::string_t& Name, std::ostream& Stream)
	{
		if(!m_emit.pipeline_value())
			return;

		const k3d::point3 from = k3d::world_position(*this);
		const k3d::point3 to = from + k3d::node_to_world_matrix(*this) * k3d::vector3(0, 0, 1);
		const k3d::color color = m_color.pipeline_value();
		const double power = m_power.pipeline_value();
		const double size = k3d::degrees(m_size.pipeline_value());
		const double nblend = m_nblend.pipeline_value();
		const double beam_falloff = m_beam_falloff.pipeline_value();

		Stream << "<!-- K-3D plugin: " << factory().name() << " name: " << name() << " -->\n";
		Stream << "<light type=\"spotlight\" name=\"" << Name << "\" power=\"" << power << "\" size=\"" << size << "\" nblend=\"" << nblend << "\" beam_falloff=\"" << beam_falloff << "\">\n";
		Stream << "	<from x=\"" << std::fixed << -from[0] << "\" y=\"" << std::fixed << from[1] << "\" z=\"" << std::fixed << from[2] << "\"/>\n";
		Stream << "	<to x=\"" << std::fixed << -to[0] << "\" y=\"" << std::fixed << to[1] << "\" z=\"" << std::fixed << to[2] << "\"/>\n";
		Stream << "	<color r=\"" << color.red << "\" g=\"" << color.green << "\" b=\"" << color.blue << "\"/>\n";
		Stream << "</light>\n";
	}

	void on_gl_draw(const k3d::gl::render_state& State)
	{
		k3d::gl::store_attributes attributes;

		k3d::gl::material(GL_FRONT_AND_BACK, GL_AMBIENT, k3d::color(0, 0, 0));
		k3d::gl::material(GL_FRONT_AND_BACK, GL_DIFFUSE, State.node_selection ? k3d::color(1, 1, 1) : k3d::color(0.4, 0.4, 0.4));
		k3d::gl::material(GL_FRONT_AND_BACK, GL_SPECULAR, k3d::color(0, 0, 0));
		k3d::gl::material(GL_FRONT_AND_BACK, GL_EMISSION, k3d::color(0, 0, 0));

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

	k3d::iplugin_factory& factory()
	{
		return get_factory();
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<spot_light,
			k3d::interface_list<k3d::imatrix_source,
			k3d::interface_list<k3d::imatrix_sink > > > factory(
				k3d::uuid(0x5e363371, 0xf8464895, 0x99f0ddf0, 0x4e26ee4a),
				"YafraySpotLight",
				_("Yafray Spot Light"),
				"Yafray Light",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	void draw_geometry()
	{
		gluQuadricDrawStyle(m_quadric, GLU_FILL);
		gluQuadricNormals(m_quadric, GLU_SMOOTH);
		gluCylinder(m_quadric, 0.001, 0.2, 1, 8, 1);
	}

	/// Stores a GLU quadric object for drawing the manipulators
	GLUquadricObj* const m_quadric;

	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_power;
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_size;
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_nblend;
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_beam_falloff;
};

k3d::iplugin_factory& spot_light_factory()
{
	return spot_light::get_factory();
}

} // namespace yafray

} // namespace module


