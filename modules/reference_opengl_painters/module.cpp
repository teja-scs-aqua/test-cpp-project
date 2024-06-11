// K-3D
// Copyright (c) 1995-2007, Timothy M. Shead
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

namespace opengl
{

namespace painters
{

extern k3d::iplugin_factory& bezier_triangle_patch_painter_factory();
extern k3d::iplugin_factory& bicubic_patch_painter_factory();
extern k3d::iplugin_factory& bilinear_patch_painter_factory();
extern k3d::iplugin_factory& blobby_point_painter_factory();
extern k3d::iplugin_factory& color_face_painter_factory();
extern k3d::iplugin_factory& cone_painter_factory();
extern k3d::iplugin_factory& cubic_curve_painter_factory();
extern k3d::iplugin_factory& cylinder_painter_factory();
extern k3d::iplugin_factory& disk_painter_factory();
extern k3d::iplugin_factory& edge_numbering_painter_factory();
extern k3d::iplugin_factory& edge_painter_factory();
extern k3d::iplugin_factory& face_normal_painter_factory();
extern k3d::iplugin_factory& face_numbering_painter_factory();
extern k3d::iplugin_factory& face_orientation_painter_factory();
extern k3d::iplugin_factory& face_painter_factory();
extern k3d::iplugin_factory& half_edge_painter_factory();
extern k3d::iplugin_factory& hyperboloid_painter_factory();
extern k3d::iplugin_factory& linear_curve_painter_factory();
extern k3d::iplugin_factory& multi_painter_factory();
extern k3d::iplugin_factory& normal_array_painter_factory();
extern k3d::iplugin_factory& nurbs_curve_numbering_painter_factory();
extern k3d::iplugin_factory& nurbs_curve_painter_factory();
extern k3d::iplugin_factory& nurbs_patch_numbering_painter_factory();
extern k3d::iplugin_factory& nurbs_patch_painter_factory();
extern k3d::iplugin_factory& paraboloid_painter_factory();
extern k3d::iplugin_factory& point_numbering_painter_factory();
extern k3d::iplugin_factory& point_painter_factory();
extern k3d::iplugin_factory& sphere_painter_factory();
extern k3d::iplugin_factory& teapot_painter_factory();
extern k3d::iplugin_factory& torus_painter_factory();
extern k3d::iplugin_factory& varying_data_painter_factory();
extern k3d::iplugin_factory& vertex_data_painter_factory();

} // namespace painters

} // namespace opengl

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::opengl::painters::bezier_triangle_patch_painter_factory());
	Registry.register_factory(module::opengl::painters::bicubic_patch_painter_factory());
	Registry.register_factory(module::opengl::painters::bilinear_patch_painter_factory());
	Registry.register_factory(module::opengl::painters::blobby_point_painter_factory());
	Registry.register_factory(module::opengl::painters::color_face_painter_factory());
	Registry.register_factory(module::opengl::painters::cone_painter_factory());
	Registry.register_factory(module::opengl::painters::cubic_curve_painter_factory());
	Registry.register_factory(module::opengl::painters::cylinder_painter_factory());
	Registry.register_factory(module::opengl::painters::disk_painter_factory());
	Registry.register_factory(module::opengl::painters::edge_numbering_painter_factory());
	Registry.register_factory(module::opengl::painters::edge_painter_factory());
	Registry.register_factory(module::opengl::painters::face_normal_painter_factory());
	Registry.register_factory(module::opengl::painters::face_numbering_painter_factory());
	Registry.register_factory(module::opengl::painters::face_orientation_painter_factory());
	Registry.register_factory(module::opengl::painters::face_painter_factory());
	Registry.register_factory(module::opengl::painters::half_edge_painter_factory());
	Registry.register_factory(module::opengl::painters::hyperboloid_painter_factory());
	Registry.register_factory(module::opengl::painters::linear_curve_painter_factory());
	Registry.register_factory(module::opengl::painters::multi_painter_factory());
	Registry.register_factory(module::opengl::painters::normal_array_painter_factory());
	Registry.register_factory(module::opengl::painters::nurbs_curve_numbering_painter_factory());
	Registry.register_factory(module::opengl::painters::nurbs_curve_painter_factory());
	Registry.register_factory(module::opengl::painters::nurbs_patch_numbering_painter_factory());
	Registry.register_factory(module::opengl::painters::nurbs_patch_painter_factory());
	Registry.register_factory(module::opengl::painters::paraboloid_painter_factory());
	Registry.register_factory(module::opengl::painters::point_numbering_painter_factory());
	Registry.register_factory(module::opengl::painters::point_painter_factory());
	Registry.register_factory(module::opengl::painters::sphere_painter_factory());
	Registry.register_factory(module::opengl::painters::teapot_painter_factory());
	Registry.register_factory(module::opengl::painters::torus_painter_factory());
	Registry.register_factory(module::opengl::painters::varying_data_painter_factory());
	Registry.register_factory(module::opengl::painters::vertex_data_painter_factory());
K3D_MODULE_END

