// K-3D
// Copyright (c) 1995-2010, Timothy M. Shead
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
#include <k3dsdk/algebra.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/mesh_simple_deformation_modifier.h>
#include <k3dsdk/noise.h>

namespace module
{

namespace deformation
{

/////////////////////////////////////////////////////////////////////////////
// linear_point_noise

class linear_point_noise :
	public k3d::mesh_simple_deformation_modifier
{
	typedef k3d::mesh_simple_deformation_modifier base;

public:
	linear_point_noise(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_use_x(init_owner(*this) + init_name("use_x") + init_label(_("Use X")) + init_description(_("Add noise on X components")) + init_value(true)),
		m_use_y(init_owner(*this) + init_name("use_y") + init_label(_("Use Y")) + init_description(_("Add noise on Y components")) + init_value(true)),
		m_use_z(init_owner(*this) + init_name("use_z") + init_label(_("Use Z")) + init_description(_("Add noise on Z components")) + init_value(true)),
		m_move_x(init_owner(*this) + init_name("move_x") + init_label(_("Move X")) + init_description(_("Apply offset on X component")) + init_value(false)),
		m_move_y(init_owner(*this) + init_name("move_y") + init_label(_("Move Y")) + init_description(_("Apply offset on Y component")) + init_value(false)),
		m_move_z(init_owner(*this) + init_name("move_z") + init_label(_("Move Z")) + init_description(_("Apply offset on Z component")) + init_value(true)),
		m_frequency_x(init_owner(*this) + init_name("frequency_x") + init_label(_("X frequency")) + init_description(_("X Frequency")) + init_value(0.1) + init_step_increment(0.01) + init_units(typeid(k3d::measurement::scalar))),
		m_frequency_y(init_owner(*this) + init_name("frequency_y") + init_label(_("Y frequency")) + init_description(_("Y Frequency")) + init_value(0.1) + init_step_increment(0.01) + init_units(typeid(k3d::measurement::scalar))),
		m_frequency_z(init_owner(*this) + init_name("frequency_z") + init_label(_("Z frequency")) + init_description(_("Z Frequency")) + init_value(0.1) + init_step_increment(0.01) + init_units(typeid(k3d::measurement::scalar))),
		m_offset_x(init_owner(*this) + init_name("offset_x") + init_label(_("X offset")) + init_description(_("X Offset")) + init_value(0.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_offset_y(init_owner(*this) + init_name("offset_y") + init_label(_("Y offset")) + init_description(_("Y Offset")) + init_value(0.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_offset_z(init_owner(*this) + init_name("offset_z") + init_label(_("Z offset")) + init_description(_("Z Offset")) + init_value(0.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_amplitude_x(init_owner(*this) + init_name("amplitude_x") + init_label(_("X amplitude")) + init_description(_("X Amplitude")) + init_value(1.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_amplitude_y(init_owner(*this) + init_name("amplitude_y") + init_label(_("Y amplitude")) + init_description(_("Y Amplitude")) + init_value(1.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_amplitude_z(init_owner(*this) + init_name("amplitude_z") + init_label(_("Z amplitude")) + init_description(_("Z Amplitude")) + init_value(1.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance)))
	{
		m_mesh_selection.changed_signal().connect(make_update_mesh_slot());
		m_use_x.changed_signal().connect(make_update_mesh_slot());
		m_use_y.changed_signal().connect(make_update_mesh_slot());
		m_use_z.changed_signal().connect(make_update_mesh_slot());
		m_move_x.changed_signal().connect(make_update_mesh_slot());
		m_move_y.changed_signal().connect(make_update_mesh_slot());
		m_move_z.changed_signal().connect(make_update_mesh_slot());
		m_frequency_x.changed_signal().connect(make_update_mesh_slot());
		m_frequency_y.changed_signal().connect(make_update_mesh_slot());
		m_frequency_z.changed_signal().connect(make_update_mesh_slot());
		m_offset_x.changed_signal().connect(make_update_mesh_slot());
		m_offset_y.changed_signal().connect(make_update_mesh_slot());
		m_offset_z.changed_signal().connect(make_update_mesh_slot());
		m_amplitude_x.changed_signal().connect(make_update_mesh_slot());
		m_amplitude_y.changed_signal().connect(make_update_mesh_slot());
		m_amplitude_z.changed_signal().connect(make_update_mesh_slot());
	}

	void on_deform_mesh(const k3d::mesh::points_t& InputPoints, const k3d::mesh::selection_t& PointSelection, k3d::mesh::points_t& OutputPoints)
	{
		const k3d::bool_t use_x = m_use_x.pipeline_value();
		const k3d::bool_t use_y = m_use_y.pipeline_value();
		const k3d::bool_t use_z = m_use_z.pipeline_value();

		const k3d::bool_t move_x = m_move_x.pipeline_value();
		const k3d::bool_t move_y = m_move_y.pipeline_value();
		const k3d::bool_t move_z = m_move_z.pipeline_value();

		const k3d::double_t frequency_x = m_frequency_x.pipeline_value();
		const k3d::double_t frequency_y = m_frequency_y.pipeline_value();
		const k3d::double_t frequency_z = m_frequency_z.pipeline_value();

		const k3d::double_t offset_x = m_offset_x.pipeline_value();
		const k3d::double_t offset_y = m_offset_y.pipeline_value();
		const k3d::double_t offset_z = m_offset_z.pipeline_value();

		const k3d::double_t amplitude_x = m_amplitude_x.pipeline_value();
		const k3d::double_t amplitude_y = m_amplitude_y.pipeline_value();
		const k3d::double_t amplitude_z = m_amplitude_z.pipeline_value();

		k3d::noise::classic3 noise;

		const k3d::uint_t point_begin = 0;
		const k3d::uint_t point_end = point_begin + OutputPoints.size();
		for(k3d::uint_t point = point_begin; point != point_end; ++point)
		{
			const k3d::point3 start = InputPoints[point];

			const k3d::vector3 perturb = k3d::noise::map3<k3d::vector3>(
				noise,
				use_x ? offset_x + frequency_x * start[0] : 0.0,
				use_y ? offset_y + frequency_y * start[1] : 0.0,
				use_z ? offset_z + frequency_z * start[2] : 0.0);

			const k3d::vector3 offset =
				k3d::vector3(
					move_x ? amplitude_x * perturb[0] : 0.0,
					move_y ? amplitude_y * perturb[1] : 0.0,
					move_z ? amplitude_z * perturb[2] : 0.0);

			OutputPoints[point] = k3d::mix(start, start + offset, PointSelection[point]);
		}
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<linear_point_noise,
			k3d::interface_list<k3d::imesh_source,
			k3d::interface_list<k3d::imesh_sink > > > factory(
				k3d::uuid(0xbbcaf2e7, 0xc45346bf, 0x9dfd92f2, 0xfb9e6d68),
				"LinearPointNoise",
				_("Applies a linear noise offset to mesh points"),
				"Deformation",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_use_x;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_use_y;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_use_z;

	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_move_x;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_move_y;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_move_z;

	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_frequency_x;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_frequency_y;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_frequency_z;

	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_offset_x;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_offset_y;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_offset_z;

	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_amplitude_x;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_amplitude_y;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_amplitude_z;
};

/////////////////////////////////////////////////////////////////////////////
// linear_point_noise_factory

k3d::iplugin_factory& linear_point_noise_factory()
{
	return linear_point_noise::get_factory();
}

} // namespace deformation

} // namespace module


