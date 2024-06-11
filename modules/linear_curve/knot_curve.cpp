// K-3D
// Copyright (c) 2002-2005, Romain Behar
//
// Contact: romainbehar@yahoo.com
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
	\author Romain Behar (romainbehar@yahoo.com)
	\author Timothy M. Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/basic_math.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/imaterial.h>
#include <k3dsdk/linear_curve.h>
#include <k3dsdk/mesh_source.h>
#include <k3dsdk/material_sink.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/node.h>
#include <k3dsdk/vectors.h>

namespace module
{

namespace linear_curve
{

/////////////////////////////////////////////////////////////////////////////
// knot_curve

class knot_curve :
	public k3d::material_sink<k3d::mesh_source<k3d::node > >
{
	typedef k3d::material_sink<k3d::mesh_source<k3d::node > > base;

public:
	knot_curve(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_edge_count(init_owner(*this) + init_name("edgecount") + init_label(_("Edge count")) + init_description(_("Number of edges building up the curve")) + init_value(100) + init_constraint(constraint::minimum<k3d::int32_t>(3)) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar))),
		m_meridian_wraps(init_owner(*this) + init_name("meridianwraps") + init_label(_("Meridian wraps")) + init_description(_("Number of meridian wraps")) + init_value(3) + init_constraint(constraint::minimum<k3d::int32_t>(1)) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar))),
		m_longitudinal_wraps(init_owner(*this) + init_name("longitudinalwraps") + init_label(_("Longitudinal wraps")) + init_description(_("Number of longitudinal wraps")) + init_value(4) + init_constraint(constraint::minimum<k3d::int32_t>(1)) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar))),
		m_scale(init_owner(*this) + init_name("scale") + init_label(_("Scale")) + init_description(_("Knot scale")) + init_value(5.0) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_width(init_owner(*this) + init_name("width") + init_label(_("Width")) + init_description(_("Curve width")) + init_value(0.1) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_wrap(init_owner(*this) + init_name("closed") + init_label(_("Closed")) + init_description(_("Closed curve (loop)")) + init_value(true))
	{
		m_edge_count.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::mesh_topology_changed> >(make_update_mesh_slot()));
		m_wrap.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::mesh_topology_changed> >(make_update_mesh_slot()));

		m_material.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_update_mesh_slot()));
		m_width.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_update_mesh_slot()));

		m_meridian_wraps.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::mesh_geometry_changed> >(make_update_mesh_slot()));
		m_longitudinal_wraps.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::mesh_geometry_changed> >(make_update_mesh_slot()));
		m_scale.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::mesh_geometry_changed> >(make_update_mesh_slot()));
	}

	void on_update_mesh_topology(k3d::mesh& Output)
	{
		const k3d::int32_t edge_count = m_edge_count.pipeline_value();
		const k3d::double_t wrap = m_wrap.pipeline_value();
		const k3d::double_t width = m_width.pipeline_value();
		k3d::imaterial* const material = m_material.pipeline_value();

		Output = k3d::mesh();

		k3d::mesh::points_t& points = Output.points.create();
		k3d::mesh::selection_t& point_selection = Output.point_selection.create();

		points.resize(edge_count);
		point_selection.assign(edge_count, 0.0);

		boost::scoped_ptr<k3d::linear_curve::primitive> primitive(k3d::linear_curve::create(Output));
		k3d::mesh::doubles_t& widths = primitive->constant_attributes.create<k3d::mesh::doubles_t>("width");

		primitive->periodic.push_back(wrap);
		primitive->material.push_back(material);
		widths.push_back(width);

		primitive->curve_first_points.push_back(primitive->curve_points.size());
		primitive->curve_point_counts.push_back(edge_count);
		primitive->curve_selections.push_back(0.0);

		for(k3d::uint32_t i = 0; i != edge_count; ++i)
			primitive->curve_points.push_back(i);
	}

	void on_update_mesh_geometry(k3d::mesh& Output)
	{
		const k3d::int32_t edge_count = m_edge_count.pipeline_value();
		const k3d::double_t mwraps = m_meridian_wraps.pipeline_value();
		const k3d::double_t lwraps = m_longitudinal_wraps.pipeline_value();
		const k3d::double_t scale = m_scale.pipeline_value();

		k3d::mesh::points_t& points = Output.points.writable();

		for(k3d::uint32_t i = 0; i != edge_count; ++i)
		{
			const k3d::double_t percent = static_cast<k3d::double_t>(i) / static_cast<k3d::double_t>(edge_count);
			const k3d::double_t radians = percent * k3d::pi_times_2();
			const k3d::double_t mu = radians * mwraps;

			k3d::point3 point;

			point[0] = cos(mu) * (1 + cos(lwraps * mu / mwraps) / 2.0);
			point[1] = sin(mu) * (1 + cos(lwraps * mu / mwraps) / 2.0);
			point[2] = sin(lwraps * mu / mwraps) / 2.0;

			points[i] = point * scale;
		}
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<knot_curve > factory(
			k3d::uuid(0xb39c53a3, 0xe2fe0e32, 0x21d09384, 0xc4ed13fe),
				"LinearKnot",
				_("Generates a PQ-knot curve"),
				"LinearCurve",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_edge_count;
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_meridian_wraps;
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_longitudinal_wraps;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_scale;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_width;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_wrap;
};

k3d::iplugin_factory& knot_curve_factory()
{
	return knot_curve::get_factory();
}

} // namespace linear_curve

} // namespace module


