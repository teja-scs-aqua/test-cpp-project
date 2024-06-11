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

#include <boost/python.hpp>

#include <k3dsdk/python/mesh_python.h>
#include <k3dsdk/python/owned_instance_wrapper_python.h>
#include <k3dsdk/python/cylinder_python.h>

#include <k3dsdk/cylinder.h>

#include <boost/python/detail/api_placeholder.hpp>
using namespace boost::python;
#include <boost/scoped_ptr.hpp>

namespace k3d
{

namespace python
{

class cylinder
{
public:
	class const_primitive
	{
	public:
		typedef owned_instance_wrapper<k3d::cylinder::const_primitive> wrapper;

		static object matrices(wrapper& Self) { return wrap(Self.wrapped().matrices); }
		static object materials(wrapper& Self) { return wrap(Self.wrapped().materials); }
		static object radii(wrapper& Self) { return wrap(Self.wrapped().radii); }
		static object z_min(wrapper& Self) { return wrap(Self.wrapped().z_min); }
		static object z_max(wrapper& Self) { return wrap(Self.wrapped().z_max); }
		static object sweep_angles(wrapper& Self) { return wrap(Self.wrapped().sweep_angles); }
		static object selections(wrapper& Self) { return wrap(Self.wrapped().selections); }
		static object constant_attributes(wrapper& Self) { return wrap(Self.wrapped().constant_attributes); }
		static object surface_attributes(wrapper& Self) { return wrap(Self.wrapped().surface_attributes); }
		static object parameter_attributes(wrapper& Self) { return wrap(Self.wrapped().parameter_attributes); }
	};

	class primitive
	{
	public:
		typedef owned_instance_wrapper<k3d::cylinder::primitive> wrapper;

		static object matrices(wrapper& Self) { return wrap(Self.wrapped().matrices); }
		static object materials(wrapper& Self) { return wrap(Self.wrapped().materials); }
		static object radii(wrapper& Self) { return wrap(Self.wrapped().radii); }
		static object z_min(wrapper& Self) { return wrap(Self.wrapped().z_min); }
		static object z_max(wrapper& Self) { return wrap(Self.wrapped().z_max); }
		static object sweep_angles(wrapper& Self) { return wrap(Self.wrapped().sweep_angles); }
		static object selections(wrapper& Self) { return wrap(Self.wrapped().selections); }
		static object constant_attributes(wrapper& Self) { return wrap(Self.wrapped().constant_attributes); }
		static object surface_attributes(wrapper& Self) { return wrap(Self.wrapped().surface_attributes); }
		static object parameter_attributes(wrapper& Self) { return wrap(Self.wrapped().parameter_attributes); }
	};


	static object create(mesh_wrapper& Mesh)
	{
		return wrap_owned(k3d::cylinder::create(Mesh.wrapped()));
	}

	static object validate(mesh_wrapper& Mesh, mesh_primitive_wrapper& Primitive)
	{
		return wrap_owned(k3d::cylinder::validate(Mesh.wrapped(), Primitive.wrapped()));
	}

	static object validate_const(const_mesh_wrapper& Mesh, const_mesh_primitive_wrapper& Primitive)
	{
		return wrap_owned(k3d::cylinder::validate(Mesh.wrapped(), Primitive.wrapped()));
	}
};

void define_namespace_cylinder()
{
	scope outer = class_<cylinder>("cylinder", no_init)
		.def("create", &cylinder::create)
		.staticmethod("create")
		.def("validate", &cylinder::validate)
		.def("validate", &cylinder::validate_const)
		.staticmethod("validate")
		;

	class_<cylinder::const_primitive::wrapper>("const_primitive", no_init)
		.def("matrices", &cylinder::const_primitive::matrices)
		.def("materials", &cylinder::const_primitive::materials)
		.def("radii", &cylinder::const_primitive::radii)
		.def("z_min", &cylinder::const_primitive::z_min)
		.def("z_max", &cylinder::const_primitive::z_max)
		.def("sweep_angles", &cylinder::const_primitive::sweep_angles)
		.def("selections", &cylinder::const_primitive::selections)
		.def("constant_attributes", &cylinder::const_primitive::constant_attributes)
		.def("surface_attributes", &cylinder::const_primitive::surface_attributes)
		.def("parameter_attributes", &cylinder::const_primitive::parameter_attributes)
		;

	class_<cylinder::primitive::wrapper>("primitive", no_init)
		.def("matrices", &cylinder::primitive::matrices)
		.def("materials", &cylinder::primitive::materials)
		.def("radii", &cylinder::primitive::radii)
		.def("z_min", &cylinder::primitive::z_min)
		.def("z_max", &cylinder::primitive::z_max)
		.def("sweep_angles", &cylinder::primitive::sweep_angles)
		.def("selections", &cylinder::primitive::selections)
		.def("constant_attributes", &cylinder::primitive::constant_attributes)
		.def("surface_attributes", &cylinder::primitive::surface_attributes)
		.def("parameter_attributes", &cylinder::primitive::parameter_attributes)
		;
}

} // namespace python

} // namespace k3d

