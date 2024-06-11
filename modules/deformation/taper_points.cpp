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
	\author Timothy M. Shead (tshead@k-3d.com)
	\author Romain Behar (romainbehar@yahoo.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/algebra.h>
#include <k3dsdk/axis.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/mesh_simple_deformation_modifier.h>
#include <k3dsdk/parallel/blocked_range.h>
#include <k3dsdk/parallel/parallel_for.h>
#include <k3dsdk/parallel/threads.h>

namespace module
{

namespace deformation
{

/////////////////////////////////////////////////////////////////////////////
// taper_points

class taper_points :
	public k3d::mesh_simple_deformation_modifier
{
	typedef k3d::mesh_simple_deformation_modifier base;

public:
	taper_points(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_axis(init_owner(*this) + init_name("axis") + init_label(_("Axis")) + init_description(_("Taper points along this axis.")) + init_value(k3d::Z) + init_enumeration(k3d::axis_values())),
		m_taper_factor(init_owner(*this) + init_name("taper_factor") + init_label(_("Taper Factor")) + init_description(_("Controls the amount of taper, 0 = no taper, 1 = taper to a point.")) + init_value(0.0) + init_step_increment(k3d::radians(1.0)) + init_units(typeid(k3d::measurement::scalar))),
		m_displace_x(init_owner(*this) + init_name("displace_x") + init_label(_("Displace X")) + init_description(_("Enable point displacement along the X axis.")) + init_value(true)),
		m_displace_y(init_owner(*this) + init_name("displace_y") + init_label(_("Displace Y")) + init_description(_("Enable point displacement along the Y axis.")) + init_value(true)),
		m_displace_z(init_owner(*this) + init_name("displace_z") + init_label(_("Displace Z")) + init_description(_("Enable point displacement along the Z axis.")) + init_value(false))
	{
		m_mesh_selection.changed_signal().connect(make_update_mesh_slot());
		m_axis.changed_signal().connect(make_update_mesh_slot());
		m_taper_factor.changed_signal().connect(make_update_mesh_slot());
		m_displace_x.changed_signal().connect(make_update_mesh_slot());
		m_displace_y.changed_signal().connect(make_update_mesh_slot());
		m_displace_z.changed_signal().connect(make_update_mesh_slot());
	}

	class worker
	{
	public:
		worker(const k3d::mesh::points_t& InputPoints, const k3d::mesh::selection_t& PointSelection, k3d::mesh::points_t& OutputPoints, const k3d::axis Axis, const double Size, const bool DisplaceX, const bool DisplaceY, const bool DisplaceZ, const double TaperFactor) :
			input_points(InputPoints),
			point_selection(PointSelection),
			output_points(OutputPoints),
			axis(Axis),
			size(Size),
			displace_x(DisplaceX),
			displace_y(DisplaceY),
			displace_z(DisplaceZ),
			taper_factor(TaperFactor)
		{
		}

		void operator()(const k3d::parallel::blocked_range<k3d::uint_t>& range) const
		{
			const k3d::uint_t point_begin = range.begin();
			const k3d::uint_t point_end = range.end();
			for(k3d::uint_t point = point_begin; point != point_end; ++point)
			{
				const double scale = k3d::mix(1.0, 1.0 - taper_factor, std::abs(input_points[point][axis] / size));

				k3d::point3 position(input_points[point]);
				if(displace_x)
					position[0] *= scale;
				if(displace_y)
					position[1] *= scale;
				if(displace_z)
					position[2] *= scale;

				output_points[point] = k3d::mix(input_points[point], position, point_selection[point]);
			}
		}

	private:
		const k3d::mesh::points_t& input_points;
		const k3d::mesh::selection_t& point_selection;
		k3d::mesh::points_t& output_points;
		const k3d::axis axis;
		const double size;
		const bool displace_x;
		const bool displace_y;
		const bool displace_z;
		const double taper_factor;
	};

	void on_deform_mesh(const k3d::mesh::points_t& InputPoints, const k3d::mesh::selection_t& PointSelection, k3d::mesh::points_t& OutputPoints)
	{

		const k3d::axis axis = m_axis.pipeline_value();
		const double taper_factor = m_taper_factor.pipeline_value();
		const bool displace_x = m_displace_x.pipeline_value();
		const bool displace_y = m_displace_y.pipeline_value();
		const bool displace_z = m_displace_z.pipeline_value();

		const k3d::bounding_box3 bounds = k3d::mesh::bounds(InputPoints);
		double size = 0.0;
		switch(axis)
		{
			case k3d::X:
				size = std::max(std::abs(bounds.px), std::abs(bounds.nx));
				break;
			case k3d::Y:
				size = std::max(std::abs(bounds.py), std::abs(bounds.ny));
				break;
			case k3d::Z:
				size = std::max(std::abs(bounds.pz), std::abs(bounds.nz));
				break;
			default:
				assert_not_reached();
		}
		if(0.0 == size)
			return;

		k3d::parallel::parallel_for(
			k3d::parallel::blocked_range<k3d::uint_t>(0, OutputPoints.size(), k3d::parallel::grain_size()),
			worker(InputPoints, PointSelection, OutputPoints, axis, size, displace_x, displace_y, displace_z, taper_factor));
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<taper_points,
			k3d::interface_list<k3d::imesh_source,
			k3d::interface_list<k3d::imesh_sink > > > factory(
				k3d::uuid(0x4d05f002, 0x27764b67, 0xa1a12e21, 0x436c3d06),
				"TaperPoints",
				_("Tapers mesh points along an axis"),
				"Deformation",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::axis, immutable_name, change_signal, with_undo, local_storage, no_constraint, enumeration_property, with_serialization) m_axis;
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_taper_factor;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_displace_x;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_displace_y;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_displace_z;
};

/////////////////////////////////////////////////////////////////////////////
// taper_points_factory

k3d::iplugin_factory& taper_points_factory()
{
	return taper_points::get_factory();
}

} // namespace deformation

} // namespace module

