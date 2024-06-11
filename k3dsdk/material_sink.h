#ifndef K3DSDK_MATERIAL_SINK_H
#define K3DSDK_MATERIAL_SINK_H

// K-3D
// Copyright (c) 1995-2004, Timothy M. Shead
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
#include <k3dsdk/data.h>
#include <k3dsdk/imaterial.h>
#include <k3dsdk/imaterial_sink.h>

namespace k3d
{

class idocument;

/// Adds a boilerplate implementation of k3d::imaterial_sink to a base class, using the parameterized-inheritance idiom
template<typename base_t>
class material_sink :
	public base_t,
	public imaterial_sink
{
public:
	material_sink(iplugin_factory& Factory, idocument& Document) :
		base_t(Factory, Document),
		m_material(init_owner(*this) + init_name("material") + init_label(_("Surface Material")) + init_description(_("Surface material")) + init_value(static_cast<imaterial*>(0)))
	{
	}

	iproperty& material_sink_input()
	{
		return m_material;
	}

protected:
	k3d_data(imaterial*, data::immutable_name, data::change_signal, data::with_undo, data::node_storage, data::no_constraint, data::node_property, data::node_serialization) m_material;
};

} // namespace k3d

#endif // !K3DSDK_MATERIAL_SINK_H

