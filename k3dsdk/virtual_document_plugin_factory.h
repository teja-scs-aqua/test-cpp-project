#ifndef K3DSDK_VIRTUAL_DOCUMENT_PLUGIN_FACTORY_H
#define K3DSDK_VIRTUAL_DOCUMENT_PLUGIN_FACTORY_H

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

#include <k3dsdk/idocument_plugin_factory.h>
#include <k3dsdk/interface_list.h>
#include <k3dsdk/plugin_factory.h>

namespace k3d
{

/////////////////////////////////////////////////////////////////////////////
// document_plugin_factory

template<typename interface_list = null_interface>
class virtual_document_plugin_factory :
	public plugin_factory,
	public idocument_plugin_factory
{
public:
	virtual_document_plugin_factory(const uuid& ClassID, const std::string& Name, const std::string& ShortDescription, const std::string& Categories, const quality_t Quality = STABLE) :
		plugin_factory(ClassID, Name, ShortDescription, Categories, Quality)
	{
	}

	bool implements(const std::type_info& InterfaceType)
	{
		return implements_interface<interface_list>()(InterfaceType);
	}

	const interfaces_t interfaces()
	{
		interfaces_t results;
		get_interfaces<interface_list>()(results);
		return results;
	}
};

} // namespace k3d

#endif // !K3DSDK_VIRTUAL_DOCUMENT_PLUGIN_FACTORY_H

