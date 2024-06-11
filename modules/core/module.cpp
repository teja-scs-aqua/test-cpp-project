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
*/

#include <k3dsdk/module.h>

namespace module
{

namespace core
{

extern k3d::iplugin_factory& axes_factory();
extern k3d::iplugin_factory& camera_factory();
extern k3d::iplugin_factory& multi_material_factory();
extern k3d::iplugin_factory& null_factory();

} // namespace core

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::core::axes_factory());
	Registry.register_factory(module::core::camera_factory());
	Registry.register_factory(module::core::multi_material_factory());
	Registry.register_factory(module::core::null_factory());
K3D_MODULE_END

