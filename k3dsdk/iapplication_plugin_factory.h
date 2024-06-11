#ifndef K3DSDK_IAPPLICATION_PLUGIN_FACTORY_H
#define K3DSDK_IAPPLICATION_PLUGIN_FACTORY_H

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
		\brief Declares iapplication_plugin_factory, an abstract factory interface for application-context plugin objects
		\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/iunknown.h>

namespace k3d
{

// Forward declarations
class iapplication;

/// Abstract factory interface for plugin objects
class iapplication_plugin_factory :
	public virtual iunknown
{
public:
	/// Creates a new instance of a plugin object
	virtual iunknown* create_plugin() = 0;

protected:
	iapplication_plugin_factory() {}
	iapplication_plugin_factory(const iapplication_plugin_factory& Other) : iunknown(Other) {}
	iapplication_plugin_factory& operator=(const iapplication_plugin_factory&) { return *this; }
	virtual ~iapplication_plugin_factory() {}
};

} // namespace k3d

#endif // !K3DSDK_IAPPLICATION_PLUGIN_FACTORY_H

