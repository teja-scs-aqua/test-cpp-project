#ifndef K3DSDK_MESH_MODIFIER_H
#define K3DSDK_MESH_MODIFIER_H

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
		\author Timothy M. Shead (tshead@k-3d.com)
		\author Romain Behar (romainbehar@yahoo.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/data.h>
#include <k3dsdk/idocument.h>
#include <k3dsdk/imesh_sink.h>
#include <k3dsdk/imesh_source.h>
#include <k3dsdk/ipipeline_profiler.h>
#include <k3dsdk/mesh.h>

namespace k3d
{

template<typename base_t>
class mesh_modifier :
	public base_t,
	public imesh_sink,
	public imesh_source
{
public:
	mesh_modifier(iplugin_factory& Factory, idocument& Document) :
		base_t(Factory, Document),
		m_input_mesh(init_owner(*this) + init_name("input_mesh") + init_label(_("Input Mesh")) + init_description(_("Input mesh")) + init_value<mesh*>(0)),
		m_output_mesh(init_owner(*this) + init_name("output_mesh") + init_label(_("Output Mesh")) + init_description(_("Output mesh")))
	{
		m_input_mesh.changed_signal().connect(make_reset_mesh_slot());

		m_output_mesh.set_initialize_slot(sigc::mem_fun(*this, &mesh_modifier<base_t>::initialize_mesh));
		m_output_mesh.set_update_slot(sigc::mem_fun(*this, &mesh_modifier<base_t>::update_mesh));
	}

	iproperty& mesh_source_output()
	{
		return m_output_mesh;
	}

	iproperty& mesh_sink_input()
	{
		return m_input_mesh;
	}

	sigc::slot<void, ihint*> make_reset_mesh_slot()
	{
		return m_output_mesh.make_reset_slot();
	}

	sigc::slot<void, ihint*> make_update_mesh_slot()
	{
		return m_output_mesh.make_update_slot();
	}

protected:
	k3d_data(mesh*, data::immutable_name, data::change_signal, data::no_undo, data::local_storage, data::no_constraint, data::read_only_property, data::no_serialization) m_input_mesh;
	k3d_data(mesh*, data::immutable_name, data::change_signal, data::no_undo, data::pointer_storage, data::no_constraint, data::read_only_property, data::no_serialization) m_output_mesh;

private:
	void initialize_mesh(mesh& Output)
	{
		if(const mesh* const input = m_input_mesh.pipeline_value())
		{
			base_t::document().pipeline_profiler().start_execution(*this, "Create Mesh");
			on_create_mesh(*input, Output);
			base_t::document().pipeline_profiler().finish_execution(*this, "Create Mesh");

			base_t::document().pipeline_profiler().start_execution(*this, "Update Mesh");
			on_update_mesh(*input, Output);
			base_t::document().pipeline_profiler().finish_execution(*this, "Update Mesh");
		}
	}

	void update_mesh(mesh& Output)
	{
		if(const mesh* const input = m_input_mesh.pipeline_value())
		{
			base_t::document().pipeline_profiler().start_execution(*this, "Update Mesh");
			on_update_mesh(*input, Output);
			base_t::document().pipeline_profiler().finish_execution(*this, "Update Mesh");
		}
	}

	virtual void on_create_mesh(const mesh& Input, mesh& Output) = 0;
	virtual void on_update_mesh(const mesh& Input, mesh& Output) = 0;
};

} // namespace k3d

#endif // !K3DSDK_MESH_MODIFIER_H

