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

#include <k3dsdk/geometry.h>
#include <k3dsdk/mesh_simple_deformation_modifier.h>

namespace k3d
{

mesh_simple_deformation_modifier::mesh_simple_deformation_modifier(iplugin_factory& Factory, idocument& Document) :
	base(Factory, Document)
{
	m_mesh_selection.changed_signal().connect(make_reset_mesh_slot());
}

void mesh_simple_deformation_modifier::on_create_mesh(const mesh& Input, mesh& Output)
{
	Output = Input;
	geometry::selection::merge(m_mesh_selection.pipeline_value(), Output);
}

void mesh_simple_deformation_modifier::on_update_mesh(const mesh& Input, mesh& Output)
{
	if(!Input.points)
		return;
	if(!Output.points)
		return;
	return_if_fail(Input.points->size() == Output.points->size());

	return_if_fail(Output.point_selection);
	return_if_fail(Output.point_selection->size() == Output.points->size());

	const mesh::points_t& input_points = *Input.points;
	const mesh::selection_t& selection = *Output.point_selection;

	document().pipeline_profiler().start_execution(*this, "Copy points");
	mesh::points_t& output_points = Output.points.writable();
	document().pipeline_profiler().finish_execution(*this, "Copy points");

	on_deform_mesh(input_points, selection, output_points);
}

} // namespace k3d

