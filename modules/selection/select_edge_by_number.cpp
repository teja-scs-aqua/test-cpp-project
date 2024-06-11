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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA

/** \file
	\author Timothy M. Shead (tshead@k-3d.com)
	\author Romain Behar (romainbehar@yahoo.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/mesh_selection_modifier.h>
#include <k3dsdk/node.h>
#include <k3dsdk/polyhedron.h>

namespace module
{

namespace selection
{

/////////////////////////////////////////////////////////////////////////////
// select_edge_by_number

class select_edge_by_number :
	public k3d::mesh_selection_modifier<k3d::node>
{
	typedef k3d::mesh_selection_modifier<k3d::node> base;

public:
	select_edge_by_number(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_primitive(init_owner(*this) + init_name("primitive") + init_label(_("Primitive Number")) + init_description(_("Primitive Number.")) + init_value(0L) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar)) + init_constraint(constraint::minimum<k3d::int32_t>(0))),
		m_index(init_owner(*this) + init_name("index") + init_label(_("Face Index")) + init_description(_("Face Index")) + init_value(0L) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar)) + init_constraint(constraint::minimum<k3d::int32_t>(0)))
	{
		m_primitive.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::selection_changed> >(make_update_mesh_slot()));

		m_index.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::selection_changed> >(make_update_mesh_slot()));
	}

	void on_update_selection(const k3d::mesh& Input, k3d::mesh& Output)
	{
		const k3d::int32_t primitive = m_primitive.pipeline_value();
		if(primitive >= Output.primitives.size())
			return;

		boost::scoped_ptr<k3d::polyhedron::primitive> polyhedron(k3d::polyhedron::validate(Output, Output.primitives[primitive]));
		if(!polyhedron)
			return;
		
		std::fill(polyhedron->edge_selections.begin(), polyhedron->edge_selections.end(), 0.0);

		const k3d::int32_t index = m_index.pipeline_value();
		if(index < polyhedron->edge_selections.size())
			polyhedron->edge_selections[index] = 1.0;
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<select_edge_by_number,
			k3d::interface_list<k3d::imesh_source,
			k3d::interface_list<k3d::imesh_sink> > > factory(
				k3d::uuid(0xbd824e9e, 0xe6440236, 0x9bb4bc9a, 0x7348e267),
				"SelectEdgeByNumber",
				_("Selects an edge from the input mesh by its index number"),
				"Selection",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_primitive;
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_index;
};

/////////////////////////////////////////////////////////////////////////////
// select_edge_by_number_factory

k3d::iplugin_factory& select_edge_by_number_factory()
{
	return select_edge_by_number::get_factory();
}

} // namespace selection

} // namespace module

