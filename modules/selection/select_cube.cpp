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

/** \file
  \author Timothy M. Shead (tshead@k-3d.com)
  \author Romain Behar (romainbehar@yahoo.com)
  */

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/mesh_selection_modifier.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/node.h>
#include <k3dsdk/renderable_gl.h>
#include <k3dsdk/transformable.h>

namespace module
{

namespace selection
{

/////////////////////////////////////////////////////////////////////////////
// select_cube

class select_cube :
	public k3d::gl::renderable<k3d::transformable<k3d::mesh_selection_modifier<k3d::node > > >
{
	typedef k3d::gl::renderable<k3d::transformable<k3d::mesh_selection_modifier<k3d::node > > > base;

public:
	select_cube(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_x1(init_owner(*this) + init_name("x1") + init_label(_("X1")) + init_description(_("X coordinate of cube's min corner")) + init_value(-10.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_y1(init_owner(*this) + init_name("y1") + init_label(_("Y1")) + init_description(_("Y coordinate of cube's min corner")) + init_value(-10.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_z1(init_owner(*this) + init_name("z1") + init_label(_("Z1")) + init_description(_("Z coordinate of cube's min corner")) + init_value(-10.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_x2(init_owner(*this) + init_name("x2") + init_label(_("X2")) + init_description(_("X coordinate of cube's max corner")) + init_value(10.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_y2(init_owner(*this) + init_name("y2") + init_label(_("Y2")) + init_description(_("Y coordinate of cube's max corner")) + init_value(10.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_z2(init_owner(*this) + init_name("z2") + init_label(_("Z2")) + init_description(_("Z coordinate of cube's max corner")) + init_value(10.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance)))
	{
		m_x1.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::selection_changed> >(make_update_mesh_slot()));
		m_y1.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::selection_changed> >(make_update_mesh_slot()));
		m_z1.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::selection_changed> >(make_update_mesh_slot()));
		m_x2.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::selection_changed> >(make_update_mesh_slot()));
		m_y2.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::selection_changed> >(make_update_mesh_slot()));
		m_z2.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::selection_changed> >(make_update_mesh_slot()));
	}

	void on_update_selection(const k3d::mesh& Input, k3d::mesh& Output)
	{
		const k3d::bounding_box3 cube(
				m_x1.pipeline_value(),
				m_x2.pipeline_value(),
				m_y1.pipeline_value(),
				m_y2.pipeline_value(),
				m_z1.pipeline_value(),
				m_z2.pipeline_value());

		if(!Input.points)
			return;
		if(!Input.point_selection)
			return;

		const k3d::mesh::points_t& points = *Output.points;
		k3d::mesh::selection_t& point_selection = Output.point_selection.writable();

		const k3d::uint_t point_begin = 0;
		const k3d::uint_t point_end = point_begin + points.size();
		for(k3d::uint_t point = point_begin; point != point_end; ++point)
			point_selection[point] = cube.contains(points[point]);
	}

	void on_gl_draw(const k3d::gl::render_state& State)
	{
		glDisable(GL_LIGHTING);
		glColor3d(1, 0, 0);

		const k3d::bounding_box3 cube(m_x1.pipeline_value(), m_x2.pipeline_value(), m_y1.pipeline_value(), m_y2.pipeline_value(), m_z1.pipeline_value(), m_z2.pipeline_value());
		if(cube.empty())
			return;

		k3d::gl::draw_bounding_box(cube);
	}

	void on_gl_select(const k3d::gl::render_state& State, const k3d::gl::selection_state& SelectState)
	{
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<select_cube,
			k3d::interface_list<k3d::imesh_source,
			k3d::interface_list<k3d::imesh_sink> > > factory(
					k3d::uuid(0xd4d45c53, 0x7dd84306, 0x909462d9, 0xa45b7f4f),
					"SelectCube",
					_("Selects portions of the input mesh that intersect a cube"),
					"Selection",
					k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_x1;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_y1;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_z1;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_x2;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_y2;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_z2;
};

/////////////////////////////////////////////////////////////////////////////
// select_cube_factory

k3d::iplugin_factory& select_cube_factory()
{
	return select_cube::get_factory();
}

} // namespace selection

} // namespace module

