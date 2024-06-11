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

namespace renderman
{

namespace painters
{

extern k3d::iplugin_factory& bicubic_patch_painter_factory();
extern k3d::iplugin_factory& bilinear_patch_painter_factory();
extern k3d::iplugin_factory& blobby_painter_factory();
extern k3d::iplugin_factory& cone_painter_factory();
extern k3d::iplugin_factory& cubic_curve_painter_factory();
extern k3d::iplugin_factory& cylinder_painter_factory();
extern k3d::iplugin_factory& disk_painter_factory();
extern k3d::iplugin_factory& hyperboloid_painter_factory();
extern k3d::iplugin_factory& linear_curve_painter_factory();
extern k3d::iplugin_factory& multi_painter_factory();
extern k3d::iplugin_factory& multi_painter_factory();
extern k3d::iplugin_factory& nurbs_patch_painter_factory();
extern k3d::iplugin_factory& paraboloid_painter_factory();
extern k3d::iplugin_factory& particle_painter_factory();
extern k3d::iplugin_factory& polyhedron_painter_factory();
extern k3d::iplugin_factory& scripted_painter_factory();
extern k3d::iplugin_factory& sphere_painter_factory();
extern k3d::iplugin_factory& subdivision_surface_painter_factory();
extern k3d::iplugin_factory& teapot_painter_factory();
extern k3d::iplugin_factory& torus_painter_factory();

} // namespace painters

} // namespace renderman

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::renderman::painters::bicubic_patch_painter_factory());
	Registry.register_factory(module::renderman::painters::bilinear_patch_painter_factory());
	Registry.register_factory(module::renderman::painters::blobby_painter_factory());
	Registry.register_factory(module::renderman::painters::cone_painter_factory());
	Registry.register_factory(module::renderman::painters::cubic_curve_painter_factory());
	Registry.register_factory(module::renderman::painters::cylinder_painter_factory());
	Registry.register_factory(module::renderman::painters::disk_painter_factory());
	Registry.register_factory(module::renderman::painters::hyperboloid_painter_factory());
	Registry.register_factory(module::renderman::painters::linear_curve_painter_factory());
	Registry.register_factory(module::renderman::painters::multi_painter_factory());
	Registry.register_factory(module::renderman::painters::nurbs_patch_painter_factory());
	Registry.register_factory(module::renderman::painters::paraboloid_painter_factory());
	Registry.register_factory(module::renderman::painters::particle_painter_factory());
	Registry.register_factory(module::renderman::painters::polyhedron_painter_factory());
	Registry.register_factory(module::renderman::painters::scripted_painter_factory());
	Registry.register_factory(module::renderman::painters::sphere_painter_factory());
	Registry.register_factory(module::renderman::painters::subdivision_surface_painter_factory());
	Registry.register_factory(module::renderman::painters::teapot_painter_factory());
	Registry.register_factory(module::renderman::painters::torus_painter_factory());
K3D_MODULE_END

