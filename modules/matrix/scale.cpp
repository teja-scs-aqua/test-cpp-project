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
		\author Tim Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/algebra.h>
#include <k3dsdk/classes.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/node.h>
#include <k3dsdk/transformable.h>

namespace module
{

namespace matrix
{

/////////////////////////////////////////////////////////////////////////////
// scale

class scale :
	public k3d::transformable<k3d::node >
{
	typedef k3d::transformable<k3d::node > base;

public:
	scale(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_space(init_owner(*this) + init_name("space") + init_label(_("Coordinate space")) + init_description(_("Coordinate space (matrix)")) + init_value(k3d::identity3())),
		m_x(init_owner(*this) + init_name("x") + init_label(_("X")) + init_description(_("X scaling")) + init_value(1.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::scalar))),
		m_y(init_owner(*this) + init_name("y") + init_label(_("Y")) + init_description(_("Y scaling")) + init_value(1.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::scalar))),
		m_z(init_owner(*this) + init_name("z") + init_label(_("Z")) + init_description(_("Z scaling")) + init_value(1.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::scalar)))
	{
		m_space.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_update_matrix_slot()));
		m_x.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_update_matrix_slot()));
		m_y.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_update_matrix_slot()));
		m_z.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_update_matrix_slot()));
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<scale,
			k3d::interface_list<k3d::imatrix_source,
			k3d::interface_list<k3d::imatrix_sink > > > factory(
				k3d::classes::Scale(),
				"MatrixScale",
				_("Creates a scale transform matrix"),
				"Matrix",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::matrix4, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_space;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_x;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_y;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_z;

	void on_update_matrix(const k3d::matrix4& Input, k3d::matrix4& Output)
	{
		Output = Input * m_space.pipeline_value() * k3d::scale3(m_x.pipeline_value(), m_y.pipeline_value(), m_z.pipeline_value()) * k3d::inverse(m_space.pipeline_value());
	}

};

/////////////////////////////////////////////////////////////////////////////
// scale_factory

k3d::iplugin_factory& scale_factory()
{
	return scale::get_factory();
}

} // namespace matrix

} // namespace module

