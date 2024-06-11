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
		\author Tim Shead <tshead@k-3d.com>
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/fstream.h>
#include <k3dsdk/inetwork_render_farm.h>
#include <k3dsdk/inetwork_render_frame.h>
#include <k3dsdk/inetwork_render_job.h>
#include <k3dsdk/inode_collection_property.h>
#include <k3dsdk/inode_collection_sink.h>
#include <k3dsdk/ipipeline.h>
#include <k3dsdk/irender_frame.h>
#include <k3dsdk/irender_preview.h>
#include <k3dsdk/network_render_farm.h>
#include <k3dsdk/node.h>
#include <k3dsdk/options.h>

#include <iomanip>
#include <iterator>

namespace module
{

namespace graphviz
{

/////////////////////////////////////////////////////////////////////////////
// render_engine

class render_engine :
	public k3d::node,
	public k3d::inode_collection_sink,
	public k3d::irender_preview,
	public k3d::irender_frame
{
	typedef k3d::node base;

public:
	render_engine(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_visible_nodes(init_owner(*this) + init_name("visible_nodes") + init_label(_("Visible Nodes")) + init_description(_("Visible Nodes")) + init_value(std::vector<k3d::inode*>())),
		m_show_property_labels(init_owner(*this) + init_name("show_property_labels") + init_label(_("Show Property Labels")) + init_description(_("Display property labels in the rendered image.")) + init_value(true)),
		m_show_property_loops(init_owner(*this) + init_name("show_property_loops") + init_label(_("Show Property Loops")) + init_description(_("Display property connections that originate and terminate on the same node.")) + init_value(true)),
		m_render_command(init_owner(*this) + init_name("render_command") + init_label(_("Render Command")) + init_description(_("Specify the GraphViz command to use for rendering.")) + init_value(k3d::string_t("dot")) + init_values(render_command_values()))
	{
	}

	const k3d::inode_collection_sink::properties_t node_collection_properties()
	{
		return k3d::inode_collection_sink::properties_t(1, &m_visible_nodes);
	}

	bool render_preview()
	{
		// Start a new render job ...
		k3d::inetwork_render_job& job = k3d::get_network_render_farm().create_job("k3d-graphviz-preview");

		// Add a single render frame to the job ...
		k3d::inetwork_render_frame& frame = job.create_frame("frame");

		// Create an output image path ...
		const k3d::filesystem::path output_image_path = frame.add_file("world.svg");

		// Render it ...
		return_val_if_fail(render(frame, output_image_path), false);

		// View the output image when it's done ...
		frame.add_view_command(output_image_path);

		// Start the job running ...
		k3d::get_network_render_farm().start_job(job);

		return true;
	}

	bool render_frame(const k3d::filesystem::path& OutputImage, const bool ViewImage)
	{
		// Sanity checks ...
		return_val_if_fail(!OutputImage.empty(), false);

		// Start a new render job ...
		k3d::inetwork_render_job& job = k3d::get_network_render_farm().create_job("k3d-graphviz-render");

		// Add a single render frame to the job ...
		k3d::inetwork_render_frame& frame = job.create_frame("frame");

		// Create an output image path ...
		const k3d::filesystem::path output_image_path = frame.add_file("world.svg");

		// Render it ...
		return_val_if_fail(render(frame, output_image_path), false);

		// Copy the output image to its requested destination ...
		frame.add_copy_command(output_image_path, OutputImage);

		// View the output image when it's done ...
		if(ViewImage)
			frame.add_view_command(OutputImage);

		// Start the job running ...
		k3d::get_network_render_farm().start_job(job);

		return true;
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<render_engine,
			k3d::interface_list<k3d::irender_frame,
			k3d::interface_list<k3d::irender_preview> > > factory(
				k3d::uuid(0xbe72cb50, 0x011f41d8, 0x90449ae0, 0x4c24ace5),
				"GraphVizEngine",
				_("GraphViz Render Engine"),
				"RenderEngine",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	template<typename T>
	static const k3d::string_t pointer_id(const T* RHS)
	{
		std::stringstream buffer;
		buffer << "v" << RHS;
		return buffer.str();
	}

	static const k3d::string_t escaped_string(const k3d::string_t& Source)
	{
		k3d::string_t result(Source);

		for(k3d::string_t::size_type i = result.find('\"'); i != k3d::string_t::npos; i = result.find('\"', i+2))
			result.replace(i, 1, "\\\"");

		return result;
	}

	bool render(k3d::inetwork_render_frame& Frame, const k3d::filesystem::path& OutputImagePath)
	{
		// Start our GraphViz DOT file ...
		const k3d::filesystem::path dot_file = Frame.add_file("world.dot");

		// Open the DOT file stream ...
		k3d::filesystem::ofstream stream(dot_file);
		return_val_if_fail(stream.good(), false);

		const k3d::bool_t show_property_labels = m_show_property_labels.pipeline_value();
		const k3d::bool_t show_property_loops = m_show_property_loops.pipeline_value();

		// Setup the frame for GraphViz rendering ...
		k3d::inetwork_render_frame::environment environment;

		k3d::inetwork_render_frame::arguments arguments;
		arguments.push_back(k3d::inetwork_render_frame::argument("-Tsvg"));
		arguments.push_back(k3d::inetwork_render_frame::argument(dot_file.native_filesystem_string()));
		arguments.push_back(k3d::inetwork_render_frame::argument("-o"));
		arguments.push_back(k3d::inetwork_render_frame::argument("world.svg"));

		Frame.add_exec_command(m_render_command.pipeline_value(), environment, arguments);

		stream << "digraph \"" << boost::any_cast<k3d::ustring>(document().title().property_internal_value()).raw() << "\"\n";
		stream << "{\n\n";
		stream << "graph [rankdir=\"LR\"]\n\n";
		stream << "node [fontname=\"Helvetica\" fontsize=\"12\" shape=\"box\" style=\"filled\" fillcolor=\"white\" width=\"0\" height=\"0\"]\n\n";
		stream << "edge [fontname=\"Helvetica\" fontsize=\"8\"]\n\n";

		// Create a mapping of properties-to-nodes as we go ...
		typedef std::map<k3d::iproperty*, k3d::inode*> property_node_map_t;
		property_node_map_t property_node_map;

		// Draw a vertex for every visible node ...
		const k3d::inode_collection_property::nodes_t visible_nodes = m_visible_nodes.pipeline_value();
		for(k3d::inode_collection_property::nodes_t::const_iterator node = visible_nodes.begin(); node != visible_nodes.end(); ++node)
		{
			stream << pointer_id(*node);
			stream << " [";
			stream << " label=\"" << escaped_string((*node)->name()) << "\"";
			stream << " ]\n";

			if(k3d::iproperty_collection* const property_collection = dynamic_cast<k3d::iproperty_collection*>(*node))
			{
				const k3d::iproperty_collection::properties_t properties = property_collection->properties();
				for(k3d::iproperty_collection::properties_t::const_iterator property = properties.begin(); property != properties.end(); ++property)
					property_node_map.insert(std::make_pair(*property, *node));
			}
		}

		// Draw an edge for every property dependency between visible nodes ...
		stream << "\n";
		const k3d::ipipeline::dependencies_t dependencies = document().pipeline().dependencies();
		for(k3d::ipipeline::dependencies_t::const_iterator dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency)
		{
			k3d::iproperty* const source_property = dependency->second;
			k3d::iproperty* const target_property = dependency->first;

			if(property_node_map.count(source_property) && property_node_map.count(target_property))
			{
				k3d::inode* const source_node = property_node_map[source_property];
				k3d::inode* const target_node = property_node_map[target_property];

				if((source_node == target_node) && !show_property_loops)
					continue;

				stream << pointer_id(source_node) << ":e -> " << pointer_id(target_node) << ":w";
				stream << " [";

				if(show_property_labels)
				{
					stream << " label=\"" << escaped_string(source_property->property_name()) << " > " << escaped_string(target_property->property_name()) << "\"";
				}

				stream << " ]\n";
			}
		}

		// Draw an edge for every property whose value is another property ...
		stream << "\n";
		for(property_node_map_t::const_iterator property = property_node_map.begin(); property != property_node_map.end(); ++property)
		{
			if(typeid(k3d::inode*) == property->first->property_type())
			{
				if(k3d::inode* const referenced_node = boost::any_cast<k3d::inode*>(property->first->property_internal_value()))
				{
					if(std::count(visible_nodes.begin(), visible_nodes.end(), referenced_node))
					{
						stream << pointer_id(referenced_node) << ":e -> " << pointer_id(property->second) << ":w";
						stream << " [ color=\"gray\"";

						if(show_property_labels)
						{
							stream << " label=\"" << escaped_string(property->first->property_name()) << "\"";
						}

						stream << " ]\n";
					}
				}
			}
		}

		stream << "\n}\n";

		return true;
	}

	k3d_data(k3d::inode_collection_property::nodes_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, node_collection_serialization) m_visible_nodes;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_show_property_labels;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_show_property_loops;
	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, list_property, with_serialization) m_render_command;

	const k3d::ilist_property<k3d::string_t>::values_t& render_command_values()
	{
		static k3d::ilist_property<k3d::string_t>::values_t values;
		if(values.empty())
		{
			values.push_back("dot");
			values.push_back("neato");
			values.push_back("twopi");
		}
		return values;
	}
};

k3d::iplugin_factory& render_engine_factory()
{
	return render_engine::get_factory();
}

} // namespace graphviz

} // namespace module

