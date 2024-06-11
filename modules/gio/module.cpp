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
		\author Timothy M. Shead (tshead@k-3d.com)
*/

#include <k3dsdk/module.h>

namespace module
{

/// Namespace reserved for the gio plugin module, to protect public symbols from name clashes with other modules
namespace gio
{

extern k3d::iplugin_factory& mime_type_handler_factory();
extern k3d::iplugin_factory& uri_handler_factory();

} // namespace gio

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::gio::mime_type_handler_factory());
	Registry.register_factory(module::gio::uri_handler_factory());
K3D_MODULE_END

