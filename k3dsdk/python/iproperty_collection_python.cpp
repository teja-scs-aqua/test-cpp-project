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
*/

#include <boost/python.hpp>

#include <k3dsdk/python/any_python.h>
#include <k3dsdk/python/iproperty_collection_python.h>
#include <k3dsdk/python/iunknown_python.h>
#include <k3dsdk/python/utility_python.h>

#include <k3dsdk/idocument.h>
#include <k3dsdk/inode.h>
#include <k3dsdk/iproperty_collection.h>
#include <k3dsdk/iwritable_property.h>
#include <k3dsdk/property.h>
#include <k3dsdk/property_types.h>
#include <k3dsdk/property_types_ri.h>
#include <k3dsdk/type_registry.h>

using namespace boost::python;

namespace k3d
{

namespace python
{

static list properties(iunknown_wrapper& Self)
{
	list results;
	const k3d::iproperty_collection::properties_t& properties = Self.wrapped<k3d::iproperty_collection>().properties();
	for(k3d::iproperty_collection::properties_t::const_iterator property = properties.begin(); property != properties.end(); ++property)
		results.append(wrap_unknown(*property));
	return results;
}

static object get_property(iunknown_wrapper& Self, const string_t& Name)
{
	return wrap_unknown(k3d::property::get(Self.wrapped<k3d::iproperty_collection>(), Name));
}

static bool has_property(iunknown_wrapper& Self, const string_t& Name)
{
	return k3d::property::get(Self.wrapped<k3d::iproperty_collection>(), Name) ? true : false;
}

void define_methods_iproperty_collection(iunknown& Interface, boost::python::object& Instance)
{
	if(!dynamic_cast<k3d::iproperty_collection*>(&Interface))
		return;

	utility::add_method(utility::make_function(&properties,
		"Returns the set of all property.held within this collection.\n\n"
		"@return: A list of L{iproperty} objects."), "properties", Instance);
	utility::add_method(utility::make_function(&get_property,
		"Returns a single property by name.\n\n"
		"@rtype: L{iproperty}\n"
		"@return: The property object if it exists, or None."), "get_property", Instance);
	utility::add_method(utility::make_function(&has_property,
		"True if the named property is registered"), "has_property", Instance);
}

} // namespace python

} // namespace k3d

