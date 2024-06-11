// K-3D
// Copyright (c) 1995-2010, Timothy M. Shead
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

namespace test
{

extern k3d::iplugin_factory& add_color_attribute_factory();
extern k3d::iplugin_factory& add_color_attributes_factory();
extern k3d::iplugin_factory& add_index_attributes_factory();
extern k3d::iplugin_factory& add_point_attributes_factory();
extern k3d::iplugin_factory& mesh_to_stdout_factory();
extern k3d::iplugin_factory& pipeline_profiler_factory();
extern k3d::iplugin_factory& selection_to_stdout_factory();
extern k3d::iplugin_factory& string_to_stdout_factory();
extern k3d::iplugin_factory& valid_meshes_factory();

} // namespace test

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::test::add_color_attribute_factory());
	Registry.register_factory(module::test::add_color_attributes_factory());
	Registry.register_factory(module::test::add_index_attributes_factory());
	Registry.register_factory(module::test::add_point_attributes_factory());
	Registry.register_factory(module::test::mesh_to_stdout_factory());
	Registry.register_factory(module::test::pipeline_profiler_factory());
	Registry.register_factory(module::test::selection_to_stdout_factory());
	Registry.register_factory(module::test::string_to_stdout_factory());
	Registry.register_factory(module::test::valid_meshes_factory());
K3D_MODULE_END

