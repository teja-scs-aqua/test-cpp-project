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

/// Namespace reserved for the selection plugin module, to protect public symbols from name clashes with other modules
namespace module
{

namespace selection
{

extern k3d::iplugin_factory& face_to_point_selections_factory();
extern k3d::iplugin_factory& grow_selection_factory();
extern k3d::iplugin_factory& make_curve_selection_factory();
extern k3d::iplugin_factory& make_edge_selection_factory();
extern k3d::iplugin_factory& make_face_selection_factory();
extern k3d::iplugin_factory& make_patch_selection_factory();
extern k3d::iplugin_factory& make_point_selection_factory();
extern k3d::iplugin_factory& mesh_selection_factory();
extern k3d::iplugin_factory& node_selection_factory();
extern k3d::iplugin_factory& select_bicubic_patch_by_number_factory();
extern k3d::iplugin_factory& select_bilinear_patch_by_number_factory();
extern k3d::iplugin_factory& select_boundary_edges_factory();
extern k3d::iplugin_factory& select_clockwise_factory();
extern k3d::iplugin_factory& select_companion_factory();
extern k3d::iplugin_factory& select_connected_components_factory();
extern k3d::iplugin_factory& select_cube_factory();
extern k3d::iplugin_factory& select_cubic_curve_by_number_factory();
extern k3d::iplugin_factory& select_degenerate_faces_factory();
extern k3d::iplugin_factory& select_edge_by_number_factory();
extern k3d::iplugin_factory& select_edgeloops_factory();
extern k3d::iplugin_factory& select_edgerings_factory();
extern k3d::iplugin_factory& select_face_by_number_factory();
extern k3d::iplugin_factory& select_linear_curve_by_number_factory();
extern k3d::iplugin_factory& select_n_sided_factory();
extern k3d::iplugin_factory& select_nurbs_curve_by_number_factory();
extern k3d::iplugin_factory& select_nurbs_patch_by_number_factory();
extern k3d::iplugin_factory& select_point_by_number_factory();
extern k3d::iplugin_factory &select_points_above_number_factory();

} // namespace selection

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::selection::face_to_point_selections_factory());
	Registry.register_factory(module::selection::grow_selection_factory());
	Registry.register_factory(module::selection::make_curve_selection_factory());
	Registry.register_factory(module::selection::make_edge_selection_factory());
	Registry.register_factory(module::selection::make_face_selection_factory());
	Registry.register_factory(module::selection::make_patch_selection_factory());
	Registry.register_factory(module::selection::make_point_selection_factory());
	Registry.register_factory(module::selection::mesh_selection_factory());
	Registry.register_factory(module::selection::node_selection_factory());
	Registry.register_factory(module::selection::select_bicubic_patch_by_number_factory());
	Registry.register_factory(module::selection::select_bilinear_patch_by_number_factory());
	Registry.register_factory(module::selection::select_boundary_edges_factory());
	Registry.register_factory(module::selection::select_clockwise_factory());
	Registry.register_factory(module::selection::select_companion_factory());
	Registry.register_factory(module::selection::select_connected_components_factory());
	Registry.register_factory(module::selection::select_cube_factory());
	Registry.register_factory(module::selection::select_cubic_curve_by_number_factory());
	Registry.register_factory(module::selection::select_degenerate_faces_factory());
	Registry.register_factory(module::selection::select_edge_by_number_factory());
	Registry.register_factory(module::selection::select_edgeloops_factory());
	Registry.register_factory(module::selection::select_edgerings_factory());
	Registry.register_factory(module::selection::select_face_by_number_factory());
	Registry.register_factory(module::selection::select_linear_curve_by_number_factory());
	Registry.register_factory(module::selection::select_n_sided_factory());
	Registry.register_factory(module::selection::select_nurbs_curve_by_number_factory());
	Registry.register_factory(module::selection::select_nurbs_patch_by_number_factory());
	Registry.register_factory(module::selection::select_point_by_number_factory());
	Registry.register_factory(module::selection::select_points_above_number_factory());
K3D_MODULE_END
