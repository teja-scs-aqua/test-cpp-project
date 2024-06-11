#ifndef MODULES_NGUI_PIPELINE_PANEL_TREE_LAYOUT_H
#define MODULES_NGUI_PIPELINE_PANEL_TREE_LAYOUT_H

// K-3D
// Copyright (c) 1995-2009, Timothy M. Shead
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

#include "graph_modifier.h"

namespace module
{

namespace ngui
{

namespace pipeline
{

class tree_layout :
	public k3d::graph_modifier
{
public:
	tree_layout();

	k3d::iproperty& column_offset()
	{
		return m_column_offset;
	}

	k3d::iproperty& row_offset()
	{
		return m_row_offset;
	}

private:
	class coordinate_visitor;

	void on_update_graph_topology(const k3d::graph::undirected& Input, k3d::graph::undirected& Output);
	void on_update_graph_attributes(const k3d::graph::undirected& Input, k3d::graph::undirected& Output);

	k3d_data(double, k3d::data::immutable_name, k3d::data::change_signal, k3d::data::no_undo, k3d::data::local_storage, k3d::data::no_constraint, k3d::data::writable_property, k3d::data::no_serialization) m_column_offset;
	k3d_data(double, k3d::data::immutable_name, k3d::data::change_signal, k3d::data::no_undo, k3d::data::local_storage, k3d::data::no_constraint, k3d::data::writable_property, k3d::data::no_serialization) m_row_offset;
};

} // namespace pipeline

} // namespace ngui

} // namespace module

#endif // !MODULES_NGUI_PIPELINE_PANEL_TREE_LAYOUT_H

