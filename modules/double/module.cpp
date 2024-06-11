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
	\author Timothy M. Shead (tshead@k-3d.com)
*/

#include <k3dsdk/module.h>

namespace module
{

namespace scalar
{

extern k3d::iplugin_factory& add_factory();
extern k3d::iplugin_factory& divide_factory();
extern k3d::iplugin_factory& expression_factory();
extern k3d::iplugin_factory& double_to_string_factory();
extern k3d::iplugin_factory& modulo_factory();
extern k3d::iplugin_factory& multiply_factory();
extern k3d::iplugin_factory& sine_factory();
extern k3d::iplugin_factory& subtract_factory();

} // namespace scalar

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::scalar::add_factory());
	Registry.register_factory(module::scalar::divide_factory());
	Registry.register_factory(module::scalar::expression_factory());
	Registry.register_factory(module::scalar::double_to_string_factory());
	Registry.register_factory(module::scalar::modulo_factory());
	Registry.register_factory(module::scalar::multiply_factory());
	Registry.register_factory(module::scalar::sine_factory());
	Registry.register_factory(module::scalar::subtract_factory());
K3D_MODULE_END

