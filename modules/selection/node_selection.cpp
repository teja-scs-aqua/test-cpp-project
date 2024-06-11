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
		\author Bart Janssens (bart.janssens@lid.kviv.be)
 */

#include <map>
#include <vector>

#include <k3d-i18n-config.h>
#include <k3dsdk/inode_selection.h>
#include <k3dsdk/ipipeline.h>
#include <k3dsdk/ipipeline_profiler.h>
#include <k3dsdk/node.h>
#include <k3dsdk/document_plugin_factory.h>

namespace module
{

namespace selection
{

class node_selection :
	public k3d::node,
	public k3d::inode_selection
{
	typedef k3d::node base;
	typedef std::map<k3d::inode*, k3d::double_t> selection_t;
public:
	node_selection(k3d::iplugin_factory& Factory, k3d::idocument& Document) : base(Factory, Document),
		m_selection_weights((init_owner(*this) + init_name("selection_weights") + init_label(_("Selection Weights")) + init_description(_("Selection weight for all nodes")) + init_value(selection_t())))
	{
		m_selection_weights.changed_signal().connect(sigc::mem_fun(*this, &node_selection::on_weights_changed));
	}

	void select(k3d::inode& Node, const k3d::double_t Weight)
	{ 
		k3d::ipipeline_profiler::profile profile(document().pipeline_profiler(), *this, "Set Node Selection");
		selection_t selection_weights = m_selection_weights.pipeline_value();
		if(Weight == 0.0) // deselect a node
		{
			if(selection_weights.erase(&Node)) // Only signal if something actually changed
				m_selection_weights.set_value(selection_weights);
		}
		else
		{
			if(!selection_weights.count(&Node) || selection_weights[&Node] != Weight)
			{
				selection_weights[&Node] = Weight;
				m_selection_weights.set_value(selection_weights);
			}
		}
		k3d::log() << debug << "--------------------------- begin document selection state --------------------------------" << std::endl;
    for(selection_t::const_iterator it = selection_weights.begin(); it != selection_weights.end(); ++it)
      k3d::log() << debug << it->first->name() << std::endl;
    k3d::log() << debug << "--------------------------- begin document selection state --------------------------------" << std::endl;
	}

	k3d::double_t selection_weight(k3d::inode& Node)
	{ 
		k3d::ipipeline_profiler::profile profile(document().pipeline_profiler(), *this, "Get Node Selection");
		selection_t selection_weights = m_selection_weights.pipeline_value();
		selection_t::const_iterator node_weight = selection_weights.find(&Node);
		if(node_weight != selection_weights.end())
		{
			return node_weight->second;
		}
		return 0.0;
	}

	const selected_nodes_t selected_nodes()
	{
		return m_selected_nodes;
	}
	
	void deselect_all()
	{
		m_selected_nodes.clear();
		selection_t empty_weights;
		m_selection_weights.set_value(empty_weights);
	}
	
	changed_signal_t& selection_changed_signal()
	{
		return m_selection_weights.changed_signal();
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<node_selection,
			k3d::interface_list<k3d::inode_selection> >factory(
			k3d::uuid(0x5d305922, 0xd2442343, 0x077956b6, 0xb112b9b3),
			"NodeSelection",
			_("Stores the document node selection state"),
			"Selection",
			k3d::iplugin_factory::STABLE);

		return factory;
	}

private:

	/// Adapts the internal ordered list of selections based on changes to the weights array
	// TODO: Use Hints to directly supply the node that was added / deleted
	void on_weights_changed(k3d::ihint* Hint)
	{
		selection_t selection_weights = m_selection_weights.pipeline_value();
		for(k3d::inode_selection::selected_nodes_t::iterator node = m_selected_nodes.begin(); node != m_selected_nodes.end(); )
		{
			if(!selection_weights.erase(*node)) // peel off all nodes that are already in the selection list
			{
				// if no node was removed, remove it from the stored list
				node = m_selected_nodes.erase(node);
			}
			else
			{
				// Just leave it in, otherwise
				++node;
			}
		}
		// All remaining nodes need to be appended to the list
		for(selection_t::const_iterator node_weight = selection_weights.begin(); node_weight != selection_weights.end(); ++node_weight)
		{
			m_selected_nodes.push_back(node_weight->first);
		}
	}

	template<typename value_t, class property_policy_t>
	class selection_weight_serialization :
		public property_policy_t,
		public ipersistent
	{
	public:
		void save(k3d::xml::element& Element, const k3d::ipersistent::save_context& Context)
		{
			std::stringstream buffer;

			const selection_t& selection = property_policy_t::internal_value();
			for(selection_t::const_iterator selection_pair = selection.begin(); selection_pair != selection.end(); ++selection_pair)
			{
				k3d::inode* node = selection_pair->first;
				k3d::double_t weight = selection_pair->second;
				if(node)
					buffer << " " << k3d::string_cast(Context.lookup.lookup_id(node));

				buffer << " " << weight;
			}

			Element.append(k3d::xml::element("property", buffer.str(), k3d::xml::attribute("name", property_policy_t::name())));
		}

		void load(k3d::xml::element& Element, const ipersistent::load_context& Context)
		{
			selection_t selection;

			std::stringstream buffer(Element.text);
			std::string node;
			std::string weight;
			while(buffer >> node)
			{
				return_if_fail(buffer >> weight);
				selection[dynamic_cast<inode*>(Context.lookup.lookup_object(k3d::from_string(node, static_cast<k3d::ipersistent_lookup::id_type>(0))))] = boost::lexical_cast<k3d::double_t>(weight);
			}

			property_policy_t::set_value(selection);
		}

	protected:
		template<typename init_t>
		selection_weight_serialization(const init_t& Init) :
			property_policy_t(Init)
		{
			Init.persistent_collection().enable_serialization(Init.name(), *this);
		}
	};

	k3d_data(selection_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, selection_weight_serialization) m_selection_weights;
	k3d::inode_selection::selected_nodes_t m_selected_nodes;
};

k3d::iplugin_factory& node_selection_factory()
{
	return node_selection::get_factory();
}

} // namespace selection

} // namespace module
