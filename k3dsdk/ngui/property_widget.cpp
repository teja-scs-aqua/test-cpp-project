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
		\brief Implements the property_widget::control, which provides a MVC UI for connecting properties in the DAG
		\author Tim Shead (tshead@k-3d.com)
		\author Romain Behar (romainbehar@yahoo.com)
*/

#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/targetlist.h>
#include <gtk/gtkmain.h>

#include <k3d-i18n-config.h>
#include <k3dsdk/color.h>
#include <k3dsdk/idocument.h>
#include <k3dsdk/imesh_source.h>
#include <k3dsdk/iproperty.h>
#include <k3dsdk/imatrix_source.h>
#include <k3dsdk/ngui/document_state.h>
#include <k3dsdk/ngui/icons.h>
#include <k3dsdk/ngui/messages.h>
#include <k3dsdk/ngui/panel_mediator.h>
#include <k3dsdk/ngui/property_widget.h>
#include <k3dsdk/ngui/utility.h>
#include <k3dsdk/ngui/widget_manip.h>
#include <k3dsdk/nodes.h>
#include <k3dsdk/property.h>
#include <k3dsdk/state_change_set.h>

namespace k3d
{

namespace ngui
{

namespace property_widget
{

/////////////////////////////////////////////////////////////////////////////
// control

control::control(std::unique_ptr<idata_proxy> Data) :
	m_data(std::move(Data)),
	m_show_connected(0),
	m_disconnect(0)
{
	assert_warning(m_data.get());
}

control::~control()
{
}

bool control::button_press_event(GdkEventButton* Event)
{
	// Open context menu with left or right mouse
	if(Event->button != 1 && Event->button != 3)
		return false;

	show_menu(true);

	return true;
}

void control::show_menu(const bool UserAction)
{
	// Reset menu
	m_disconnect = 0;
	m_menu_items.clear();
	m_show_connected = 0;
	m_menu.reset(new Gtk::Menu());

	// If the property's connected ...
	k3d::iproperty* const dependency = m_data->document().document().pipeline().dependency(m_data->property());
	if(dependency)
	{
		k3d::inode* const node = k3d::find_node(m_data->document().document().nodes(), *dependency);

		m_show_connected = new Gtk::MenuItem(_("Show Connected"));
		m_show_connected << connect_menu_item(sigc::bind(sigc::mem_fun(*this, &control::on_show_connected), node));
		m_menu->items().push_back(*Gtk::manage(m_show_connected));

		m_disconnect = new Gtk::MenuItem(_("Disconnect"));
		m_disconnect << connect_menu_item(sigc::bind(sigc::mem_fun(*this, &control::on_disconnect), node));
		m_menu->items().push_back(*Gtk::manage(m_disconnect));
	}

	// Get the list of compatible properties
	Gtk::Menu* const connect_submenu = new Gtk::Menu();
	m_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Connect to ..."), *manage(connect_submenu)));

	typedef std::map<std::string, Gtk::Menu*> node_menu_items_t;
	node_menu_items_t items_map;

	for(k3d::inode_collection::nodes_t::const_iterator node = m_data->document().document().nodes().collection().begin(); node != m_data->document().document().nodes().collection().end(); ++node)
	{
		k3d::iproperty_collection* const property_collection = dynamic_cast<k3d::iproperty_collection*>(*node);
		if(!property_collection)
			continue;

		typedef std::map<std::string, k3d::iproperty*> properties_t;
		properties_t properties_map;

		k3d::iproperty_collection::properties_t all_properties = property_collection->properties();
		for(k3d::iproperty_collection::properties_t::iterator p = all_properties.begin(); p != all_properties.end(); ++p)
		{
			if(m_data->property().property_type() != (*p)->property_type())
				continue;

			const std::string label = (*p)->property_label();
			properties_map.insert(std::make_pair(label, *p));
		}

		if(!properties_map.size())
			continue;

		// Add compatible properties for this node to the 'connect' submenu
		Gtk::Menu* const node_submenu = new Gtk::Menu();
		items_map.insert(std::make_pair((*node)->name(), node_submenu));

		for(properties_t::const_iterator property = properties_map.begin(); property != properties_map.end(); ++property)
		{
			Gtk::MenuItem* const menu_item = new Gtk::MenuItem(property->first);
			menu_item << connect_menu_item(sigc::bind(sigc::mem_fun(*this, &control::on_connect_to), property->second));
			m_menu_items.insert(std::make_pair(property->second, menu_item));

			node_submenu->items().push_back(*Gtk::manage(menu_item));
		}
	}

	// Sort nodes by name
	for(node_menu_items_t::const_iterator item = items_map.begin(); item != items_map.end(); ++item)
	{
		connect_submenu->items().push_back(Gtk::Menu_Helpers::MenuElem(item->first, *manage(item->second)));
	}
	
	m_menu->show_all();

	if(UserAction)
		m_menu->popup(1, gtk_get_current_event_time());
	else
		m_menu->popup(0, GDK_CURRENT_TIME);
}

bool control::button_release_event(GdkEventButton* Event)
{
	if(Event->button != 1)
		return false;

	return true;
}

void control::on_show_connected(k3d::inode* Node)
{
	show_connected(Node);
}

void control::show_connected(k3d::inode* Node)
{
	return_if_fail(Node);
	panel::mediator(m_data->document().document()).set_focus(*Node);
}

void control::on_connect_to(k3d::iproperty* Property)
{
	k3d::record_state_change_set changeset(m_data->document().document(), _("Connect Properties"), K3D_CHANGE_SET_CONTEXT);
	connect_to(Property);
}

void control::connect_to(k3d::iproperty* Property)
{
	return_if_fail(Property);

	// If the property's already connected, disconnect it first
	if(m_data->document().document().pipeline().dependency(m_data->property()))
	{
		k3d::ipipeline::dependencies_t dependencies;
		dependencies.insert(std::make_pair(&m_data->property(), static_cast<k3d::iproperty*>(0)));
		m_data->document().document().pipeline().set_dependencies(dependencies);
	}

	// Make connection
	k3d::ipipeline::dependencies_t dependencies;
	dependencies.insert(std::make_pair(&m_data->property(), Property));
	m_data->document().document().pipeline().set_dependencies(dependencies);
}

void control::on_disconnect(k3d::inode* Node)
{
	k3d::record_state_change_set changeset(m_data->document().document(), m_data->change_message + " Disconnect", K3D_CHANGE_SET_CONTEXT);
	disconnect(Node);
}

void control::disconnect(k3d::inode* Node)
{
	return_if_fail(Node);

	k3d::ipipeline::dependencies_t dependencies;
	dependencies.insert(std::make_pair(&m_data->property(), static_cast<k3d::iproperty*>(0)));
	m_data->document().document().pipeline().set_dependencies(dependencies);
}

} // namespace property_widget

} // namespace ngui

} // namespace k3d

