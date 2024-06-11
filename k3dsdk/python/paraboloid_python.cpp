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
#include <k3dsdk/python/paraboloid_python.h>

#include <k3dsdk/paraboloid.h>

#include <boost/python/detail/api_placeholder.hpp>
using namespace boost::python;
#include <boost/scoped_ptr.hpp>

namespace k3d
{

namespace python
{

class paraboloid
{
public:
	class const_primitive
	{
	public:
		typedef owned_instance_wrapper<k3d::paraboloid::const_primitive> wrapper;

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
		typedef owned_instance_wrapper<k3d::paraboloid::primitive> wrapper;

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
		return wrap_owned(k3d::paraboloid::create(Mesh.wrapped()));
	}

	static object validate(mesh_wrapper& Mesh, mesh_primitive_wrapper& Primitive)
	{
		return wrap_owned(k3d::paraboloid::validate(Mesh.wrapped(), Primitive.wrapped()));
	}

	static object validate_const(const_mesh_wrapper& Mesh, const_mesh_primitive_wrapper& Primitive)
	{
		return wrap_owned(k3d::paraboloid::validate(Mesh.wrapped(), Primitive.wrapped()));
	}
};

void define_namespace_paraboloid()
{
	scope outer = class_<paraboloid>("paraboloid", no_init)
		.def("create", &paraboloid::create)
		.staticmethod("create")
		.def("validate", &paraboloid::validate)
		.def("validate", &paraboloid::validate_const)
		.staticmethod("validate")
		;

	class_<paraboloid::const_primitive::wrapper>("const_primitive", no_init)
		.def("matrices", &paraboloid::const_primitive::matrices)
		.def("materials", &paraboloid::const_primitive::materials)
		.def("radii", &paraboloid::const_primitive::radii)
		.def("z_min", &paraboloid::const_primitive::z_min)
		.def("z_max", &paraboloid::const_primitive::z_max)
		.def("sweep_angles", &paraboloid::const_primitive::sweep_angles)
		.def("selections", &paraboloid::const_primitive::selections)
		.def("constant_attributes", &paraboloid::const_primitive::constant_attributes)
		.def("surface_attributes", &paraboloid::const_primitive::surface_attributes)
		.def("parameter_attributes", &paraboloid::const_primitive::parameter_attributes)
		;

	class_<paraboloid::primitive::wrapper>("primitive", no_init)
		.def("matrices", &paraboloid::primitive::matrices)
		.def("materials", &paraboloid::primitive::materials)
		.def("radii", &paraboloid::primitive::radii)
		.def("z_min", &paraboloid::primitive::z_min)
		.def("z_max", &paraboloid::primitive::z_max)
		.def("sweep_angles", &paraboloid::primitive::sweep_angles)
		.def("selections", &paraboloid::primitive::selections)
		.def("constant_attributes", &paraboloid::primitive::constant_attributes)
		.def("surface_attributes", &paraboloid::primitive::surface_attributes)
		.def("parameter_attributes", &paraboloid::primitive::parameter_attributes)
		;
}

} // namespace python

} // namespace k3d

