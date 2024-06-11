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
	\brief Implements the DocumentReader K-3D plugin, which imports the K-3D native file format
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/algebra.h>
#include <k3dsdk/application_plugin_factory.h>
#include <k3dsdk/classes.h>
#include <k3dsdk/data.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/gzstream.h>
#include <k3dsdk/idocument.h>
#include <k3dsdk/idocument_importer.h>
#include <k3dsdk/idocument_plugin_factory.h>
#include <k3dsdk/inode_collection.h>
#include <k3dsdk/ipipeline_profiler.h>
#include <k3dsdk/log.h>
#include <k3dsdk/metadata_keys.h>
#include <k3dsdk/persistent_lookup.h>
#include <k3dsdk/serialization_xml.h>
#include <k3dsdk/string_modifiers.h>
#include <k3dsdk/vectors.h>
#include <k3dsdk/xml.h>

#include <boost/assign/list_of.hpp>

#include <iostream>

namespace module
{

namespace k3d_io
{

/////////////////////////////////////////////////////////////////////////////
// document_importer
	
static void node_execution(k3d::inode& Node, const k3d::string_t& Task, k3d::double_t Time)
{
	k3d::log() << info << Node.name() << " " << Task << " " << Time << std::endl;
}

class document_importer :
	public k3d::idocument_importer
{
public:
	k3d::imetadata::metadata_t get_file_metadata(const k3d::filesystem::path& File)
	{
		k3d::imetadata::metadata_t metadata;

		try
		{
			k3d::xml::element xml("k3dml");
			k3d::filesystem::igzstream stream(File);
			k3d::xml::hide_progress progress;
			k3d::xml::parse(xml, stream, File.native_utf8_string().raw(), progress);

			metadata.insert(std::make_pair(k3d::metadata::key::version(), k3d::xml::attribute_text(xml, "version")));
		}
		catch(std::exception& e)
		{
			k3d::log() << error << e.what() << std::endl;
		}

		return metadata;
	}

	k3d::bool_t read_file(const k3d::filesystem::path& File, k3d::idocument& Document)
	{
		k3d::log() << info << "Reading " << File.native_console_string() << " using " << get_factory().name() << std::endl;

//		sigc::connection connection = Document.pipeline_profiler().connect_node_execution_signal(sigc::ptr_fun(&node_execution));

		k3d::xml::element xml("k3dml");
		try
		{
			k3d::filesystem::igzstream stream(File);
			k3d::xml::hide_progress progress;
			k3d::xml::parse(xml, stream, File.native_utf8_string().raw(), progress);
		}
		catch(std::exception& e)
		{
			k3d::log() << error << e.what() << std::endl;
			return false;
		}

		// Make sure it's a K3D document ...
		return_val_if_fail(xml.name == "k3dml", false);

		const k3d::filesystem::path root_path = File.branch_path();
		k3d::persistent_lookup persistent_lookup;
		k3d::ipersistent::load_context context(root_path, persistent_lookup);

		// Load per-node data ...
		if(k3d::xml::element* xml_document = k3d::xml::find_element(xml, "document"))
		{
			// Handle documents from older versions of the software by modifying the XML
			k3d::xml::upgrade_document(*xml_document);

			// Load nodes
			if(k3d::xml::element* xml_nodes = k3d::xml::find_element(*xml_document, "nodes"))
			{
				k3d::inode_collection::nodes_t nodes;
				std::vector<k3d::inode*> persistent_nodes;
				std::vector<k3d::xml::element*> node_storage;

				for(k3d::xml::element::elements_t::iterator xml_node = xml_nodes->children.begin(); xml_node != xml_nodes->children.end(); ++xml_node)
				{
					if(xml_node->name != "node")
						continue;

					if(k3d::xml::attribute_value<k3d::bool_t>(*xml_node, "do_not_load", false))
						continue;

					const std::string name = k3d::xml::attribute_text(*xml_node, "name");
					const k3d::uuid factory_id = k3d::xml::attribute_value<k3d::uuid>(*xml_node, "factory", k3d::uuid::null());
					if(factory_id == k3d::uuid::null())
					{
						k3d::log() << error << "node [" << name << "] with unspecified factory ID will not be loaded" << std::endl;
						continue;
					}

					const k3d::ipersistent_lookup::id_type node_id = k3d::xml::attribute_value<k3d::ipersistent_lookup::id_type>(*xml_node, "id", 0);
					if(node_id == 0)
					{
						k3d::log() << error << "node [" << name << "] with unspecified ID will not be loaded" << std::endl;
						continue;
					}

					k3d::iplugin_factory* const plugin_factory = k3d::plugin::factory::lookup(factory_id);
					if(!plugin_factory)
					{
						k3d::log() << error << "node [" << name << "] with unknown factory ID [" << factory_id << "] will not be loaded" << std::endl;
						continue;
					}

					k3d::idocument_plugin_factory* const document_plugin_factory = dynamic_cast<k3d::idocument_plugin_factory*>(plugin_factory);
					if(!document_plugin_factory)
					{
						k3d::log() << error << "Non-document plugin [" << name << "] will not be loaded" << std::endl;
						continue;
					}

					k3d::inode* const node = document_plugin_factory->create_plugin(*plugin_factory, Document);
					if(!node)
					{
						k3d::log() << error << "Error creating node [" << name << "] instance" << std::endl;
						continue;
					}

					k3d::ipersistent* const persistent = dynamic_cast<k3d::ipersistent*>(node);
					if(!persistent)
					{
						k3d::log() << error << "node [" << name << "] does not support persistence" << std::endl;

						delete node;
						continue;
					}

					k3d::undoable_new(node, Document);

					nodes.push_back(node);
					persistent_nodes.push_back(node);
					node_storage.push_back(&(*xml_node));

					persistent_lookup.insert_lookup(node_id, node);
				}

				Document.nodes().add_nodes(nodes);

				for(k3d::uint_t i = 0; i != persistent_nodes.size(); ++i)
					k3d::xml::load(*persistent_nodes[i], *node_storage[i], context);
			}

			// Load the DAG ...
			k3d::xml::load_pipeline(Document, *xml_document, context);
		}

//		connection.disconnect();

		return true;
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::application_plugin_factory<document_importer, k3d::interface_list<k3d::idocument_importer> > factory(
			k3d::classes::DocumentImporter(),
			"K3DDocumentImporter",
			_("K-3D Native ( .k3d )"),
			"DocumentImporter",
			k3d::iplugin_factory::STABLE,
			boost::assign::map_list_of("k3d:mime-types", "application/x-k3d"));

		return factory;
	}
};

k3d::iplugin_factory& document_importer_factory()
{
	return document_importer::get_factory();
}

} // namespace k3d_io

} // namespace module

