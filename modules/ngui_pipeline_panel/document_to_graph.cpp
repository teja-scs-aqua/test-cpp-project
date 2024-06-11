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
// License along with this program; if not, read to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/** \file
	\author Timothy M. Shead
*/

#include "document_to_graph.h"

#include <k3dsdk/imaterial.h>
#include <k3dsdk/imesh_painter_gl.h>
#include <k3dsdk/imesh_painter_ri.h>
#include <k3dsdk/ipipeline.h>

namespace module
{

namespace ngui
{

namespace pipeline
{

document_to_graph::document_to_graph(k3d::idocument& Document) :
	m_document(Document),
	m_include_materials(init_owner(*this) + init_name("include_materials") + init_label("") + init_description("") + init_value(false)),
	m_include_painters(init_owner(*this) + init_name("include_painters") + init_label("") + init_description("") + init_value(false))
{
	m_include_materials.changed_signal().connect(k3d::hint::converter<
		k3d::hint::convert<k3d::hint::any, k3d::hint::graph_topology_changed> >(make_update_graph_slot()));

	m_include_painters.changed_signal().connect(k3d::hint::converter<
		k3d::hint::convert<k3d::hint::any, k3d::hint::graph_topology_changed> >(make_update_graph_slot()));
}

void document_to_graph::on_update_graph_topology(k3d::graph::undirected& Output)
{
	const bool include_materials = m_include_materials.pipeline_value();
	const bool include_painters = m_include_painters.pipeline_value();

	const k3d::nodes_t nodes = m_document.nodes().collection();

	k3d::graph::undirected::adjacency_list_t& topology = Output.topology.create();
	k3d::graph::nodes_t& vertex_node = Output.vertex_data.create<k3d::graph::nodes_t>("node");
	k3d::graph::indices_t& edge_type = Output.edge_data.create<k3d::graph::indices_t>("type");

	// Insert nodes ...
	std::map<k3d::inode*, k3d::graph::undirected::vertex_descriptor_t> node_map;
	for(k3d::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
	{
		if(!include_materials && dynamic_cast<k3d::imaterial*>(*node))
			continue;
		if(!include_painters && dynamic_cast<k3d::gl::imesh_painter*>(*node))
			continue;
		if(!include_painters && dynamic_cast<k3d::ri::imesh_painter*>(*node))
			continue;

		node_map[*node] = boost::add_vertex(topology);
		vertex_node.push_back(*node);
	}

	// Insert edges ...
	for(k3d::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
	{
		if(!node_map.count(*node))
			continue;

		if(k3d::iproperty_collection* const property_collection = dynamic_cast<k3d::iproperty_collection*>(*node))
		{
			const k3d::iproperty_collection::properties_t properties = property_collection->properties();
			for(k3d::iproperty_collection::properties_t::const_iterator property = properties.begin(); property != properties.end(); ++property)
			{
				if(typeid(k3d::inode*) == (*property)->property_type())
				{
					if(k3d::inode* const referenced_node = boost::any_cast<k3d::inode*>((*property)->property_internal_value()))
					{
						if(!node_map.count(referenced_node))
							continue;

						boost::add_edge(node_map[referenced_node], node_map[*node], topology).first;
						edge_type.push_back(BEHAVIOR_EDGE);
					}
				}
			}
		}
	}

	const k3d::ipipeline::dependencies_t dependencies = m_document.pipeline().dependencies();
	for(k3d::ipipeline::dependencies_t::const_iterator dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency)
	{
		if(dependency->first && dependency->first->property_node() && dependency->second && dependency->second->property_node())
		{
			if(!node_map.count(dependency->second->property_node()))
				continue;

			if(!node_map.count(dependency->first->property_node()))
				continue;

			boost::add_edge(node_map[dependency->second->property_node()], node_map[dependency->first->property_node()], topology).first;
			edge_type.push_back(DATA_EDGE);
		}
	}
}

void document_to_graph::on_update_graph_attributes(k3d::graph::undirected& Output)
{
}

} // namespace pipeline

} // namespace ngui

} // namespace module

