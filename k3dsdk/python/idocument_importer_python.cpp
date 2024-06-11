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

#include <k3dsdk/python/idocument_importer_python.h>
#include <k3dsdk/python/idocument_python.h>
#include <k3dsdk/python/iunknown_python.h>
#include <k3dsdk/python/utility_python.h>

#include <k3dsdk/idocument_importer.h>
#include <k3dsdk/path.h>

using namespace boost::python;

namespace k3d
{

namespace python
{

///////////////////////////////////////////////////////////////////////////////////////////////
// idocument_importer

static void get_file_metadata(iunknown_wrapper& Self, const filesystem::path& Path)
{
	Self.wrapped<k3d::idocument_importer>().get_file_metadata(Path);
}

static bool_t read_file(iunknown_wrapper& Self, const filesystem::path& Path, idocument_wrapper& Document)
{
	return Self.wrapped<k3d::idocument_importer>().read_file(Path, Document.wrapped());
}

void define_methods_idocument_importer(iunknown& Interface, boost::python::object& Instance)
{
	if(!dynamic_cast<k3d::idocument_importer*>(&Interface))
		return;

	utility::add_method(make_function(read_file), "read_file", Instance);
}

} // namespace python

} // namespace k3d

