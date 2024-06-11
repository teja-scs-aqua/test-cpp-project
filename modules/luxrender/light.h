#ifndef MODULES_LUXRENDER_LIGHT_H
#define MODULES_LUXRENDER_LIGHT_H

// K-3D
// Copyright (c) 1995-2009, Timothy M. Shead
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
	\author Tim Shead <tshead@k-3d.com>
*/

#include <k3dsdk/data.h>
#include <k3dsdk/node.h>

#include <iosfwd>

namespace module
{

namespace luxrender
{

/////////////////////////////////////////////////////////////////////////////
// light

/// Abstract interface for LuxRender light nodes.
class light :
	public k3d::node
{
public:
	light(k3d::iplugin_factory& Factory, k3d::idocument& Document);

	void setup(std::ostream& Stream);

private:
	/// Implemented in derivatives to do the actual work of inserting the light definition into the scene.
	virtual void on_setup(std::ostream& Stream) = 0;

	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_group;
};

} // namespace luxrender

} // namespace module

#endif // !MODULES_LUXRENDER_LIGHT_H
