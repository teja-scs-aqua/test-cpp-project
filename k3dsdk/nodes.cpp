// K-3D
// Copyright (c) 1995-2005, Timothy M. Shead
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

#include <k3dsdk/data.h>
#include <k3dsdk/idocument.h>
#include <k3dsdk/inode_collection_sink.h>
#include <k3dsdk/ipipeline.h>
#include <k3dsdk/iplugin_factory.h>
#include <k3dsdk/iproperty_collection.h>
#include <k3dsdk/istate_recorder.h>
#include <k3dsdk/nodes.h>
#include <k3dsdk/property.h>
#include <k3dsdk/string_cast.h>
#include <k3dsdk/string_modifiers.h>
#include <k3dsdk/utility.h>

// The following includes are needed to compare typeinfo of properties in skip_nodes
#include <k3dsdk/mesh.h>

namespace k3d
{

namespace detail
{

template<typename functor_t>
struct factory_id_filter_t
{
	explicit factory_id_filter_t(const uuid FactoryID, functor_t Functor) : factory_id(FactoryID), functor(Functor) {}

	void operator()(k3d::inode* Object) { if(Object->factory().factory_id() == factory_id) functor(Object); }

	const uuid factory_id;
	functor_t functor;
};

template<typename functor_t>
factory_id_filter_t<functor_t> factory_id_filter(const uuid ID, functor_t Functor)
{
	return factory_id_filter_t<functor_t>(ID, Functor);
}

void skip_node(inode& Node, ipipeline::dependencies_t& NewDependencies)
{
	idocument& document = Node.document();
	ipipeline& pipeline = document.pipeline();
	try
	{
		iproperty_collection& property_collection = dynamic_cast<iproperty_collection&>(Node);
		iproperty_collection::properties_t properties = property_collection.properties();
		for(iproperty_collection::properties_t::const_iterator property = properties.begin(); property != properties.end(); ++property)
		{
			iproperty* connected_property = pipeline.dependency(**property);
			if(!connected_property)
				continue;
			
			for(ipipeline::dependencies_t::const_iterator dependency = pipeline.dependencies().begin(); dependency != pipeline.dependencies().end(); ++dependency)
			{
				iproperty* output_property = dependency->second;
				if(output_property && output_property->property_node() == &Node && 
						output_property->property_name() == connected_property->property_name() && output_property->property_type() == connected_property->property_type())
				{
					NewDependencies[dependency->first] = connected_property;
				}
			}
		}
	}
	catch(std::bad_cast& E) // Catch the exception when the dynamic cast to ioperty_collection fails
	{
		return;
	}
}

void skip_nodes(nodes_t Nodes, ipipeline::dependencies_t& NewDependencies)
{
	for(nodes_t::iterator node = Nodes.begin(); node != Nodes.end(); ++node)
		skip_node(**node, NewDependencies);
}

} // namespace detail

inode* find_node(inode_collection& Nodes, iproperty& Property)
{
	const nodes_t::const_iterator end = Nodes.collection().end();
	for(nodes_t::const_iterator node = Nodes.collection().begin(); node != end; ++node)
	{
		iproperty_collection* const property_collection = dynamic_cast<iproperty_collection*>(*node);
		if(!property_collection)
			continue;

		const iproperty_collection::properties_t& properties = property_collection->properties();
		if(std::find(properties.begin(), properties.end(), &Property) != properties.end())
			return *node;
	}

	return 0;
}

const std::string unique_name(inode_collection& Nodes, const std::string& Name)
{
	// For each node in the collection ...
	for(k3d::inode_collection::nodes_t::const_iterator handle = Nodes.collection().begin(); handle != Nodes.collection().end(); ++handle)
	{
		inode* const node = *handle;

		// Name doesn't match, so keep going ...
		if(Name != node->name())
			continue;

		// Got a duplicate name, so recursively try something else ...
		std::string base(k3d::trim(Name));
		unsigned int copy = 1;

		//parse(Name.c_str(), lexeme_d[*(anychar_p - space_p)][assign(base)] >> !(int_p[assign(copy)]), space_p);

		// Find trailing space followed by a number and increment ('k3d 5' -> 'k3d 6', 'k3d3' -> 'k3d3 2')
		std::string::iterator c = base.end();
		while(--c != base.begin() && *c >= '0' && *c <= '9')
		{
		}

		if(*c == ' ')
		{
			// Get trailing number
			std::string number(c + 1, base.end());
			copy = k3d::from_string<unsigned long>(number, 1);

			// Cut trailing space + number
			base = std::string(base.begin(), c);
		}

		return unique_name(Nodes, base + ' ' + k3d::string_cast(copy+1));
	}

	return Name;
}

void delete_nodes(idocument& Document, const nodes_t& Nodes)
{
	ipipeline::dependencies_t skip_dependencies;
	// Get the dependencies needed to skip the deleted node
	detail::skip_nodes(Nodes, skip_dependencies);
	// Let the nodes know that they're about to be deleted ...
	for(nodes_t::const_iterator node = Nodes.begin(); node != Nodes.end(); ++node)
		(*node)->deleted_signal().emit();

	// Remove them from the document node collection ...
	Document.nodes().remove_nodes(Nodes);
	
	// Remove them from node collection sinks
	const k3d::inode_collection::nodes_t::const_iterator doc_node_end = Document.nodes().collection().end();
	for(k3d::inode_collection::nodes_t::const_iterator doc_node = Document.nodes().collection().begin(); doc_node != doc_node_end; ++doc_node)
	{
		if(k3d::inode_collection_sink* const node_collection_sink = dynamic_cast<k3d::inode_collection_sink*>(*doc_node))
		{
			const k3d::inode_collection_sink::properties_t properties = node_collection_sink->node_collection_properties();
			for(k3d::inode_collection_sink::properties_t::const_iterator property = properties.begin(); property != properties.end(); ++property)
			{
				if(k3d::inode_collection_property* const node_collection_property = dynamic_cast<k3d::inode_collection_property*>(*property))
				{
					k3d::inode_collection_property::nodes_t nodes = k3d::property::internal_value<k3d::inode_collection_property::nodes_t>(**property);
					for(nodes_t::const_iterator node = Nodes.begin(); node != Nodes.end(); ++node)
					{
						for (k3d::inode_collection_property::nodes_t::iterator visible_node = nodes.begin(); visible_node != nodes.end(); )
						{
							if (*visible_node == *node)
								visible_node = nodes.erase(visible_node);
							else
								++visible_node;
						}
					}
					k3d::property::set_internal_value(**property, nodes);
				}
			}
		}
	}

	// Make sure the node gets cleaned-up properly after a redo ...
	for(nodes_t::const_iterator node = Nodes.begin(); node != Nodes.end(); ++node)
		k3d::undoable_delete(*node, Document);
	
	Document.pipeline().set_dependencies(skip_dependencies);
}

void make_visible(const nodes_t& Nodes, inode_collection_sink& NodeCollectionSink)
{
	const k3d::inode_collection_sink::properties_t properties = NodeCollectionSink.node_collection_properties();
	for(k3d::inode_collection_sink::properties_t::const_iterator property = properties.begin(); property != properties.end(); ++property)
	{
		if(k3d::inode_collection_property* const node_collection_property = dynamic_cast<k3d::inode_collection_property*>(*property))
		{
			k3d::inode_collection_property::nodes_t nodes = k3d::property::internal_value<k3d::inode_collection_property::nodes_t>(**property);
			nodes.insert(nodes.end(), Nodes.begin(), Nodes.end());
		}
	}
}

} // namespace k3d

