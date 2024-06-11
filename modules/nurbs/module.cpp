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

#include <k3dsdk/module.h>

namespace module
{

namespace nurbs
{

extern k3d::iplugin_factory& add_trim_curve_factory();
extern k3d::iplugin_factory& close_curve_factory();
extern k3d::iplugin_factory& connect_curves_factory();
extern k3d::iplugin_factory& create_cap_factory();
extern k3d::iplugin_factory& curve_degree_elevation_factory();
extern k3d::iplugin_factory& curve_traversal_factory();
extern k3d::iplugin_factory& edit_knot_vector_factory();
extern k3d::iplugin_factory& extract_curve_point_factory();
extern k3d::iplugin_factory& extract_patch_curve_factory();
extern k3d::iplugin_factory& extract_trim_curves_factory();
extern k3d::iplugin_factory& extrude_curve_factory();
extern k3d::iplugin_factory& extrude_patch_factory();
extern k3d::iplugin_factory& flip_curve_factory();
extern k3d::iplugin_factory& insert_knot_factory();
extern k3d::iplugin_factory& merge_connected_curves_factory();
extern k3d::iplugin_factory& merge_curve_knot_vectors_factory();
extern k3d::iplugin_factory& patch_degree_elevation_factory();
extern k3d::iplugin_factory& patch_insert_knot_factory();
extern k3d::iplugin_factory& polygonize_curve_factory();
extern k3d::iplugin_factory& polygonize_patch_factory();
extern k3d::iplugin_factory& revolve_curve_factory();
extern k3d::iplugin_factory& ruled_surface_factory();
extern k3d::iplugin_factory& set_weight_factory();
extern k3d::iplugin_factory& skinned_surface_factory();
extern k3d::iplugin_factory& split_curve_factory();
extern k3d::iplugin_factory& split_patch_factory();
extern k3d::iplugin_factory& sweep_surface_factory();

} // namespace nurbs

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::nurbs::add_trim_curve_factory());
	Registry.register_factory(module::nurbs::close_curve_factory());
	Registry.register_factory(module::nurbs::connect_curves_factory());
	Registry.register_factory(module::nurbs::create_cap_factory());
	Registry.register_factory(module::nurbs::curve_degree_elevation_factory());
	Registry.register_factory(module::nurbs::curve_traversal_factory());
	Registry.register_factory(module::nurbs::edit_knot_vector_factory());
	Registry.register_factory(module::nurbs::extract_curve_point_factory());
	Registry.register_factory(module::nurbs::extract_patch_curve_factory());
	Registry.register_factory(module::nurbs::extract_trim_curves_factory());
	Registry.register_factory(module::nurbs::extrude_curve_factory());
	Registry.register_factory(module::nurbs::extrude_patch_factory());
	Registry.register_factory(module::nurbs::flip_curve_factory());
	Registry.register_factory(module::nurbs::insert_knot_factory());
	Registry.register_factory(module::nurbs::merge_connected_curves_factory());
	Registry.register_factory(module::nurbs::merge_curve_knot_vectors_factory());
	Registry.register_factory(module::nurbs::patch_degree_elevation_factory());
	Registry.register_factory(module::nurbs::patch_insert_knot_factory());
	Registry.register_factory(module::nurbs::polygonize_curve_factory());
	Registry.register_factory(module::nurbs::polygonize_patch_factory());
	Registry.register_factory(module::nurbs::revolve_curve_factory());
	Registry.register_factory(module::nurbs::ruled_surface_factory());
	Registry.register_factory(module::nurbs::set_weight_factory());
	Registry.register_factory(module::nurbs::skinned_surface_factory());
	Registry.register_factory(module::nurbs::split_curve_factory());
	Registry.register_factory(module::nurbs::split_patch_factory());
	Registry.register_factory(module::nurbs::sweep_surface_factory());
K3D_MODULE_END

