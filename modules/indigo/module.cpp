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
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/module.h>

namespace module
{

/// Namespace reserved for the indigo plugin module, to protect public symbols from name clashes with other modules
namespace indigo
{

extern k3d::iplugin_factory& background_factory();
extern k3d::iplugin_factory& diffuse_material_factory();
extern k3d::iplugin_factory& rectangle_light_factory();
extern k3d::iplugin_factory& render_engine_factory();
extern k3d::iplugin_factory& skylight_factory();

} // namespace indigo

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::indigo::background_factory());
	Registry.register_factory(module::indigo::diffuse_material_factory());
	Registry.register_factory(module::indigo::rectangle_light_factory());
	Registry.register_factory(module::indigo::render_engine_factory());
	Registry.register_factory(module::indigo::skylight_factory());
K3D_MODULE_END

