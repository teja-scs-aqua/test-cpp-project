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
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/property_group_collection.h>

namespace k3d
{

/////////////////////////////////////////////////////////////////////////////
// property_group_collection

property_group_collection::property_group_collection()
{
}

property_group_collection::~property_group_collection()
{
}

const iproperty_group_collection::groups_t property_group_collection::property_groups()
{
	return m_groups;
}

void property_group_collection::register_property_group(const group& Group)
{
	m_groups.push_back(Group);
}

void property_group_collection::unregister_property_group(const std::string& Name)
{
	groups_t::iterator group = m_groups.begin();
	while (group != m_groups.end())
	{
		if (group->name == Name)
		{
			group = m_groups.erase(group);
		}
		else
		{
			++group;
		}
	}
}

void property_group_collection::clear()
{
	m_groups.clear();
}

} // namespace k3d


