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
		\brief Implements Bulge tool, applies a "Bulge" transformation to points
		\author Andy Gill (buzz@ucky.com)
		\author Tim Shead (tshead@k-3d.com)
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
// bulge_points

class bulge_points :
	public k3d::mesh_simple_deformation_modifier
{
	typedef k3d::mesh_simple_deformation_modifier base;

public:
	bulge_points(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_bulge_factor(init_owner(*this) + init_name("bulge_factor") + init_label(_("Bulge factor")) + init_description(_("Bulge amount")) + init_value(1.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::scalar))),
		m_origin(init_owner(*this) + init_name("origin") + init_label(_("Origin")) + init_description(_("Origin of the bulge.")) + init_value(k3d::point3(0, 0, 0))),
		m_displace_x(init_owner(*this) + init_name("displace_x") + init_label(_("Displace x")) + init_description(_("Displace on X axis")) + init_value(true)),
		m_displace_y(init_owner(*this) + init_name("displace_y") + init_label(_("Displace y")) + init_description(_("Displace on Y axis")) + init_value(true)),
		m_displace_z(init_owner(*this) + init_name("displace_z") + init_label(_("Displace z")) + init_description(_("Displace on Z axis")) + init_value(true)),
		m_axis(init_owner(*this) + init_name("axis") + init_label(_("Axis")) + init_description(_("Axis to bulge along")) + init_value(k3d::Z) + init_enumeration(k3d::axis_values())),
		m_type(init_owner(*this) + init_name("type") + init_label(_("Type")) + init_description(_("Bulge type")) + init_value(RADIAL) + init_enumeration(type_values()))
	{
		m_mesh_selection.changed_signal().connect(make_update_mesh_slot());
		m_origin.changed_signal().connect(make_update_mesh_slot());
		m_bulge_factor.changed_signal().connect(make_update_mesh_slot());
		m_displace_x.changed_signal().connect(make_update_mesh_slot());
		m_displace_y.changed_signal().connect(make_update_mesh_slot());
		m_displace_z.changed_signal().connect(make_update_mesh_slot());
		m_axis.changed_signal().connect(make_update_mesh_slot());
		m_type.changed_signal().connect(make_update_mesh_slot());
	}

	class worker
	{
	public:
		worker(const k3d::mesh::points_t& InputPoints, const k3d::mesh::selection_t& PointSelection, k3d::mesh::points_t& OutputPoints, const k3d::point3& Origin, const k3d::point3& Min, const k3d::point3& Max, const k3d::axis Axis, const k3d::bool_t Radial, const k3d::bool_t displace_x, const k3d::bool_t displace_y, const k3d::bool_t displace_z, const k3d::double_t bulge_factor) :
			input_points(InputPoints),
			point_selection(PointSelection),
			output_points(OutputPoints),
			m_origin(k3d::to_vector(Origin)),
			m_min(Min),
			m_max(Max),
			m_axis(Axis),
			m_radial(Radial),
			m_displace_x(displace_x),
			m_displace_y(displace_y),
			m_displace_z(displace_z),
			m_bulge_factor(bulge_factor),
			m_size(Max - Min)
		{
		}

		void operator()(const k3d::parallel::blocked_range<k3d::uint_t>& range) const
		{
			const k3d::uint_t point_begin = range.begin();
			const k3d::uint_t point_end = range.end();
			for(k3d::uint_t point = point_begin; point != point_end; ++point)
			{
				k3d::point3 coords = input_points[point];

				coords -= m_origin;

				k3d::double_t delta = 0;
				k3d::double_t length = m_size[m_axis];
				if(length != 0)
				{
					k3d::double_t min = m_min[m_axis];
					k3d::double_t max = m_max[m_axis];
					k3d::double_t value = coords[m_axis];

					k3d::double_t down = value - min;
					k3d::double_t up = max - value;

					delta = 2 * down * up / (length*length);
				}

				k3d::double_t bulge_amount = delta;
				if(m_radial)
				{
					k3d::double_t distance = k3d::to_vector(coords).length();
					k3d::double_t scale;
					if(0 == distance)
						scale = 1.0;
					else
						scale = (distance + m_bulge_factor * bulge_amount) / distance;

					if(m_displace_x && (k3d::X != m_axis))
						coords[0] *= scale;

					if(m_displace_y && (k3d::Y != m_axis))
						coords[1] *= scale;

					if(m_displace_z && (k3d::Z != m_axis))
						coords[2] *= scale;
				}
				else
				{
					k3d::double_t offset = m_bulge_factor * bulge_amount;

					if(m_displace_x && (k3d::X != m_axis))
						coords[0] += offset;

					if(m_displace_y && (k3d::Y != m_axis))
						coords[1] += offset;

					if(m_displace_z && (k3d::Z != m_axis))
						coords[2] += offset;
				}

				coords += m_origin;

				output_points[point] = k3d::mix(input_points[point], coords, point_selection[point]);
			}
		}

	private:
		const k3d::mesh::points_t& input_points;
		const k3d::mesh::selection_t& point_selection;
		k3d::mesh::points_t& output_points;
		const k3d::vector3 m_origin;
		const k3d::point3 m_min;
		const k3d::point3 m_max;
		const k3d::axis m_axis;
		const k3d::bool_t m_radial;
		const k3d::bool_t m_displace_x;
		const k3d::bool_t m_displace_y;
		const k3d::bool_t m_displace_z;
		const k3d::double_t m_bulge_factor;
		const k3d::vector3 m_size;
	};

	void on_deform_mesh(const k3d::mesh::points_t& InputPoints, const k3d::mesh::selection_t& PointSelection, k3d::mesh::points_t& OutputPoints)
	{
		const k3d::bounding_box3 bounds = k3d::mesh::bounds(InputPoints);

		const k3d::double_t bulge_factor = m_bulge_factor.pipeline_value();
		const k3d::double_t displace_x = m_displace_x.pipeline_value();
		const k3d::double_t displace_y = m_displace_y.pipeline_value();
		const k3d::double_t displace_z = m_displace_z.pipeline_value();
		const k3d::axis axis = m_axis.pipeline_value();

		const k3d::point3 origin = m_origin.pipeline_value();
		const k3d::point3 min(bounds.nx, bounds.ny, bounds.nz);
		const k3d::point3 max(bounds.px, bounds.py, bounds.pz);
		const k3d::bool_t radial = m_type.pipeline_value() == RADIAL;

		k3d::parallel::parallel_for(
			k3d::parallel::blocked_range<k3d::uint_t>(0, OutputPoints.size(), k3d::parallel::grain_size()),
			worker(InputPoints, PointSelection, OutputPoints, origin, min, max, axis, radial, displace_x, displace_y, displace_z, bulge_factor));
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<bulge_points,
			k3d::interface_list<k3d::imesh_source,
			k3d::interface_list<k3d::imesh_sink > > > factory(
				k3d::uuid(0xb7002ece, 0x8e6348f5, 0xa99ce9b0, 0xfbeba55f),
				"BulgePoints",
				_("Bulges mesh points around a point"),
				"Deformation",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	typedef enum
	{
		LINEAR,
		RADIAL,
	} Type;

	friend std::ostream& operator<<(std::ostream& Stream, const Type& Value)
	{
		switch(Value)
		{
			case LINEAR:
				Stream << "linear";
				break;
			case RADIAL:
				Stream << "radial";
				break;
		}

		return Stream;
	}

	friend std::istream& operator>>(std::istream& Stream, Type& Value)
	{
		std::string text;
		Stream >> text;

		if(text == "linear")
			Value = LINEAR;
		else if(text == "radial")
			Value = RADIAL;
		else
			k3d::log() << error << k3d_file_reference << ": unknown enumeration [" << text << "]" << std::endl;

		return Stream;
	}

	static const k3d::ienumeration_property::enumeration_values_t& type_values()
	{
		static k3d::ienumeration_property::enumeration_values_t values;
		if(values.empty())
		{
			values.push_back(k3d::ienumeration_property::enumeration_value_t("Linear", "linear", "Linear bulge"));
			values.push_back(k3d::ienumeration_property::enumeration_value_t("Radial", "radial", "Radial bulge"));
		}

		return values;
	}

	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_bulge_factor;
	k3d_data(k3d::point3, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_origin;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_displace_x;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_displace_y;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_displace_z;
	k3d_data(k3d::axis, immutable_name, change_signal, with_undo, local_storage, no_constraint, enumeration_property, with_serialization) m_axis;
	k3d_data(Type, immutable_name, change_signal, with_undo, local_storage, no_constraint, enumeration_property, with_serialization) m_type;
};

/////////////////////////////////////////////////////////////////////////////
// bulge_points_factory

k3d::iplugin_factory& bulge_points_factory()
{
	return bulge_points::get_factory();
}

} // namespace deformation

} // namespace module


