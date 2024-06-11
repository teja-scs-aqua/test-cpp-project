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
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/metadata_keys.h>

namespace k3d
{

namespace metadata
{

namespace key
{

const string_t authors()
{
	return "k3d:authors";
}

const string_t copyright()
{
	return "k3d:copyright";
}

const string_t domain()
{
	return "k3d:domain";
}

const string_t role()
{
	return "k3d:role";
}

const string_t version()
{
	return "k3d:version";
}

} // namespace key

namespace value
{

const string_t point_indices_domain()
{
	return "k3d:point-indices";
}

const string_t multi_line_text_role()
{
	return "k3d:multi-line-text";
}

const string_t selection_role()
{
	return "k3d:selection";
}

const string_t nurbs_knot_vector_role()
{
	return "k3d:nurbs-knot-vector";
}

} // namespace value

} // namespace metadata

} // namespace k3d

