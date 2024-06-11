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

#include <k3dsdk/ngui/angle_axis.h>
#include <k3dsdk/ngui/auto_property_page.h>
#include <k3dsdk/ngui/bitmap_preview.h>
#include <k3dsdk/ngui/bounding_box.h>
#include <k3dsdk/ngui/button.h>
#include <k3dsdk/ngui/check_button.h>
#include <k3dsdk/ngui/collapsible_frame.h>
#include <k3dsdk/ngui/color_chooser.h>
#include <k3dsdk/ngui/combo_box.h>
#include <k3dsdk/ngui/custom_property_control.h>
#include <k3dsdk/ngui/document_state.h>
#include <k3dsdk/ngui/entry.h>
#include <k3dsdk/ngui/enumeration_chooser.h>
#include <k3dsdk/ngui/node_chooser.h>
#include <k3dsdk/ngui/node_collection_chooser.h>
#include <k3dsdk/ngui/path_chooser.h>
#include <k3dsdk/ngui/point3.h>
#include <k3dsdk/ngui/property_button.h>
#include <k3dsdk/ngui/property_label.h>
#include <k3dsdk/ngui/script_button.h>
#include <k3dsdk/ngui/spin_button.h>
#include <k3dsdk/ngui/text.h>
#include <k3dsdk/ngui/widget_manip.h>

#include <k3d-i18n-config.h>
#include <k3dsdk/application.h>
#include <k3dsdk/iapplication.h>
#include <k3dsdk/imetadata.h>
#include <k3dsdk/metadata_keys.h>
#include <k3dsdk/istate_recorder.h>
#include <k3dsdk/iuser_property.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/state_change_set.h>
#include <k3dsdk/type_registry.h>
#include <k3dsdk/user_property.h>
#include <k3dsdk/utility.h>

// Not strictly required to compile, but this #include ensures that we have a std::typeinfo for k3d::mesh that matches the SDK (i.e. we don't break the ODR)
#include <k3dsdk/mesh.h>

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>

namespace k3d
{

namespace ngui
{

namespace auto_property_page
{

///////////////////////////////////////////////////////////////////////////
// control::implementation

class control::implementation
{
public:
	implementation(document_state& DocumentState) :
		m_document_state(DocumentState)
	{
	}

	void set_properties(const objects_t& Objects)
	{
		iproperty_group_collection::groups_t final_groups;

		if(Objects.size() == 1) // As a special case when displaying a single object ...
		{
			// Get the node properties, grouped together ...
			if(iproperty_collection* const property_collection = dynamic_cast<iproperty_collection*>(Objects[0]))
			{
				iproperty_group_collection::group builtin_properties(_("Builtin Properties"));
				iproperty_group_collection::group user_properties(_("User Properties"));

				const iproperty_collection::properties_t properties = property_collection->properties();
				for(iproperty_collection::properties_t::const_iterator property = properties.begin(); property != properties.end(); ++property)
				{
					if(dynamic_cast<iuser_property*>(*property))
						user_properties.properties.push_back(*property);
					else
						builtin_properties.properties.push_back(*property);
				}

				if(iproperty_group_collection* const property_group_collection = dynamic_cast<iproperty_group_collection*>(Objects[0]))
				{
					const iproperty_group_collection::groups_t groups = property_group_collection->property_groups();
					for(iproperty_group_collection::groups_t::const_iterator group = groups.begin(); group != groups.end(); ++group)
					{
						for(iproperty_collection::properties_t::const_iterator property = group->properties.begin(); property != group->properties.end(); ++property)
						{
							builtin_properties.properties.erase(
								std::remove(builtin_properties.properties.begin(), builtin_properties.properties.end(), *property),
								builtin_properties.properties.end());
						}

						if(!group->properties.empty())
							final_groups.push_back(*group);
					}
				}

				if(!builtin_properties.properties.empty())
					final_groups.insert(final_groups.begin(), builtin_properties);
				if(!user_properties.properties.empty())
					final_groups.push_back(user_properties);
			}
		}
		else // Display multiple objects ...
		{
			// For each object ...
			for(objects_t::const_iterator object = Objects.begin(); object != Objects.end(); ++object)
			{
				// Create a property group ...
				iproperty_group_collection::group group(_("Unknown object type"));

				// Assign the group a name based on the node name or the factory name ...
				if(inode* const node = dynamic_cast<inode*>(*object))
				{
					group.name = node->name();

					if(group.name.empty())
						group.name = node->factory().name();
				}

				if(iproperty_collection* const property_collection = dynamic_cast<iproperty_collection*>(*object))
					group.properties = property_collection->properties();

				final_groups.push_back(group);
			}
		}

		set_properties(final_groups);
	}

	void set_properties(const iproperty_group_collection::groups_t& PropertyGroups)
	{
		// Delete existing controls ...
		Glib::ListHandle<Gtk::Widget*> children = m_vbox.get_children();
		std::for_each(children.begin(), children.end(), delete_object());

		istate_recorder* const state_recorder = &m_document_state.document().state_recorder();

		for(iproperty_group_collection::groups_t::const_iterator group = PropertyGroups.begin(); group != PropertyGroups.end(); ++group)
		{
			collapsible_frame::control* const frame = new collapsible_frame::control(group->name, m_collapsible_frame_group);
			frame->get_accessible()->set_name(group->name);
			m_vbox.pack_start(*manage(frame), Gtk::PACK_SHRINK);

			Gtk::Table* const table = new Gtk::Table(group->properties.size(), 5, false);
			frame->add(*manage(table));

			// Store entries for focus chain within table
			std::list<Gtk::Widget*> entry_list;

			const unsigned long prop_delete_begin = 0;
			const unsigned long prop_delete_end = 1;
			const unsigned long prop_button_begin = 1;
			const unsigned long prop_button_end = 2;
			const unsigned long prop_label_begin = 2;
			const unsigned long prop_label_end = 3;
			const unsigned long prop_control_begin = 3;
			const unsigned long prop_control_end = 4;

			// For each property within the group ...
			unsigned int row = 0;
			for(unsigned int i = 0; i != group->properties.size(); ++i, ++row)
			{
				iproperty& property = *group->properties[i];

				const string_t property_name = property.property_name();
				const std::type_info& property_type = property.property_type();
				
				// Provide a property button for the property ...
				table->attach(*Gtk::manage(
					new property_button::control(property_widget::proxy(m_document_state,property))),
					prop_button_begin, prop_button_end, row, row + 1, Gtk::SHRINK, Gtk::SHRINK);

				// Provide a label for the property ...
				table->attach(*Gtk::manage(
					new property_label::control(property_widget::proxy(m_document_state, property))),
					prop_label_begin, prop_label_end, row, row + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);

				// Keep track of the control that's created (if any) ...
				Gtk::Widget* control = 0;

				// Look for custom property controls that match our property role ...
				if(!control)
				{
					// Look for an explicit property role ...
					if(imetadata* const property_metadata = dynamic_cast<imetadata*>(&property))
					{
						const string_t property_control_role = property_metadata->get_metadata()[k3d::metadata::key::role()];
						if(property_control_role.size())
						{
							// Now see if we can find a custom property control that matches the role ...
							static plugin::factory::collection_t control_factories = plugin::factory::lookup("ngui:component-type", "property-control");
							for(plugin::factory::collection_t::const_iterator factory = control_factories.begin(); factory != control_factories.end(); ++factory)
							{
								if((**factory).metadata()["ngui:property-role"] != property_control_role)
									continue;

								if(custom_property::control* const custom_control = plugin::create<custom_property::control>(**factory))
								{
									if(control = dynamic_cast<Gtk::Widget*>(custom_control))
									{
										custom_control->initialize(m_document_state, property);
									}
									else
									{
										k3d::log() << error << "custom property control must derive from Gtk::Widget" << std::endl;
									}
								}
								else
								{
									k3d::log() << error << "error creating custom property control" << std::endl;
								}

								break;
							}
						}
					}
				}

				// Look for custom property controls that match our property type ...
				if(!control)
				{
					// Get the C++ property type ...
					const string_t property_control_type = k3d::type_string(property_type);
					if(property_control_type.size())
					{
						// Now see if we can find a custom property control that matches the type ...
						static plugin::factory::collection_t control_factories = plugin::factory::lookup("ngui:component-type", "property-control");
						for(plugin::factory::collection_t::const_iterator factory = control_factories.begin(); factory != control_factories.end(); ++factory)
						{
							if((**factory).metadata()["ngui:property-type"] != property_control_type)
								continue;

							if(custom_property::control* const custom_control = plugin::create<custom_property::control>(**factory))
							{
								if(control = dynamic_cast<Gtk::Widget*>(custom_control))
								{
									custom_control->initialize(m_document_state, property);
								}
								else
								{
									k3d::log() << error << "custom property control must derive from Gtk::Widget" << std::endl;
								}
							}
							else
							{
								k3d::log() << error << "error creating custom property control" << std::endl;
							}

							break;
						}
					}
				}

				// Otherwise, provide our own hard-wired controls based on property type ...
				if(!control)
				{
					// Boolean properties ...
					if(property_type == typeid(bool_t))
					{
						control = new check_button::control(check_button::proxy(property, state_recorder, property_name));
					}
					// Scalar properties ...
					else if(property_type == typeid(int32_t) || property_type == typeid(uint32_t) || property_type == typeid(float_t) || property_type == typeid(double_t))
					{
						control = new spin_button::control(spin_button::model(property), state_recorder);
					}
					// Color properties ...
					else if(property_type == typeid(color))
					{
						control = new color_chooser::control(color_chooser::proxy(property, state_recorder, property_name));
					}
					// String properties ...
					else if(property_type == typeid(string_t))
					{
						if(dynamic_cast<ienumeration_property*>(&property))
						{
							control = new enumeration_chooser::control(enumeration_chooser::model(property), state_recorder);
						}
						else if(dynamic_cast<iscript_property*>(&property))
						{
							control = new script_button::control(script_button::model(property), state_recorder, property_name);
						}
						else if(ilist_property<string_t>* const list_property = dynamic_cast<ilist_property<string_t>*>(&property))
						{
							combo_box::control* const combo_box = new combo_box::control(combo_box::proxy(property, state_recorder, property_name));
							combo_box->set_values(list_property->property_values());
							control = combo_box;
						}
						else
						{
							if(imetadata* const metadata = dynamic_cast<imetadata*>(&property))
							{
								imetadata::metadata_t property_metadata = metadata->get_metadata();
								if(property_metadata["k3d:property-type"] == "k3d:multi-line-text")
								{
									control = new text::control(text::model(property), state_recorder);
								}
							}

							if(!control)
							{
								control = new entry::control(entry::model(property), state_recorder);
							}
						}
					}
					// inode* properties ...
					else if(property_type == typeid(inode*))
					{
						control = new node_chooser::control(node_chooser::proxy(m_document_state, property, state_recorder, property_name), node_chooser::filter(property));
					}
					else if(property_type == typeid(inode_collection_property::nodes_t))
					{
						control = new node_collection_chooser::control(node_collection_chooser::model(property), state_recorder);
					}
					// Bitmap properties ...
					else if(property_type == type_id_k3d_bitmap_ptr())
					{
						control = new bitmap_preview::control(bitmap_preview::proxy(property));
					}
					// Filesystem-path properties ...
					else if(property_type == typeid(filesystem::path))
					{
						control = new path_chooser::control(path_chooser::proxy(property, state_recorder, property_name));
					}
					// bounding_box3 properties ...
					else if(property_type == typeid(bounding_box3))
					{
						control = new bounding_box::control(bounding_box::proxy(property));
					}
					// point3 properties ...
					else if(property_type == typeid(point3) || property_type == typeid(vector3) || property_type == typeid(normal3))
					{
						control = new point::control(point::proxy(property));
					}
					// angle_axis properties ...
					else if(property_type == typeid(k3d::angle_axis))
					{
						control = new k3d::ngui::angle_axis::control(k3d::ngui::angle_axis::proxy(property, state_recorder, property_name));
					}
					// Transformation properties ...
					else if(property_type == typeid(matrix4))
					{
					}
					// Mesh properties ...
					else if(property_type == typeid(mesh*))
					{
					}
					// HPoint properties ...
					else if(property_type == typeid(point4))
					{
					}
					// Pipeline-profiler records ...
					else if(property_type == typeid(std::map<inode*, std::map<string_t, double_t> >))
					{
					}
					else
					{
						log() << warning << k3d_file_reference << "unknown property type: " << property_type.name() << " name: " << property_name << std::endl;
					}
				}

				// Pack the new control into the rest of the UI
				if(control)
				{
					control->get_accessible()->set_name(property_name + "_control");
					table->attach(*manage(control), prop_control_begin, prop_control_end, row, row + 1, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK);
					entry_list.push_back(control);
				}

				// Provide a "delete" button for user properties ...
				if(dynamic_cast<iuser_property*>(&property))
				{
					iproperty_collection* const property_collection = dynamic_cast<iproperty_collection*>(property.property_node());

					Gtk::Button* const control =
						button::create(*Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON)))
						<< connect_button(sigc::bind(sigc::bind(sigc::mem_fun(*this, &implementation::on_delete_user_property), &property), property_collection))
						<< set_tooltip(_("Delete user property (no undo)"));

					table->attach(*manage(control), prop_delete_begin, prop_delete_end, row, row + 1, Gtk::SHRINK, Gtk::SHRINK);
				}
			}

			// Set focus chain ...
			table->set_focus_chain(entry_list);
		}

		// Display everything ...
		m_vbox.show_all();
	}

	void on_delete_user_property(iproperty_collection* Collection, iproperty* Property)
	{
		return_if_fail(Collection);
		return_if_fail(Property);
		return_if_fail(dynamic_cast<iuser_property*>(Property));

		record_state_change_set change_set(m_document_state.document(), "Delete user property", K3D_CHANGE_SET_CONTEXT);

		if(m_document_state.document().state_recorder().current_change_set())
			m_document_state.document().state_recorder().current_change_set()->record_old_state(new user::property_container(*Collection));

		Collection->unregister_property(*Property);
		if(ipersistent* const persistent = dynamic_cast<ipersistent*>(Property))
		{
			if(ipersistent_collection* const persistent_collection = dynamic_cast<ipersistent_collection*>(Collection))
				persistent_collection->disable_serialization(*persistent);
		}

		undoable_delete(Property, m_document_state.document());

		if(m_document_state.document().state_recorder().current_change_set())
			m_document_state.document().state_recorder().current_change_set()->record_new_state(new user::property_container(*Collection));
	}

	document_state& m_document_state;
	/// Stores the collection of property controls
	Gtk::VBox m_vbox;
	/// Groups collapsible frames together
	collapsible_frame::group m_collapsible_frame_group;
};

///////////////////////////////////////////////////////////////////////////
// control

control::control(document_state& DocumentState) :
	m_implementation(new implementation(DocumentState))
{
}

control::~control()
{
	delete m_implementation;
}

void control::set_properties(iunknown* Object)
{
	m_implementation->set_properties(objects_t(1, Object));
}

void control::set_properties(const objects_t& Objects)
{
	m_implementation->set_properties(Objects);
}

void control::set_properties(const iproperty_group_collection::groups_t& PropertyGroups)
{
	m_implementation->set_properties(PropertyGroups);
}

Gtk::Widget& control::get_widget()
{
	return m_implementation->m_vbox;
}

} // namespace auto_property_page

} // namespace ngui

} // namespace k3d

