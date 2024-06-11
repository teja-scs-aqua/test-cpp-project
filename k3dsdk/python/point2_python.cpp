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

#include <boost/python.hpp>

#include <k3dsdk/python/point2_python.h>
#include <k3dsdk/python/utility_python.h>

#include <k3dsdk/vectors.h>

#include <boost/python/detail/api_placeholder.hpp>
using namespace boost::python;

namespace k3d
{

namespace python
{

void define_class_point2()
{
	class_<k3d::point2>("point2",
		"Stores a position in two-dimensional space", no_init)
		.def(init<double, double>())
		.def(init<const k3d::point2&>())
		.def("__len__", &utility::constant_len_len<k3d::point2, 2>)
		.def("__getitem__", &utility::constant_len_get_item<k3d::point2, 2, k3d::double_t>)
		.def("__setitem__", &utility::constant_len_set_item<k3d::point2, 2, k3d::double_t>)
		.def(self == self)
		.def(self != self)
		.def(self + self)
		.def(self - self)
		.def(self * double())
		.def(double() * self)
		.def(self *= double())
		.def(self /= double())
		.def(self += k3d::vector2())
		.def(self -= k3d::vector2())
		.def(self_ns::str(self));
}

} // namespace python

} // namespace k3d

