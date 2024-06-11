// K-3D
// Copyright (c) 1995-2005, Timothy M. Shead
//
// Contact: tshead@k-3d.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your argument) any later version.
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

#include <k3dsdk/ngui/icons.h>
#include <k3dsdk/ngui/menus.h>
#include <k3dsdk/ngui/widget_manip.h>

#include <k3d-i18n-config.h>
#include <k3dsdk/iplugin_factory.h>
#include <k3dsdk/string_cast.h>

#include <gtkmm/accellabel.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/imagemenuitem.h>

#include <boost/format.hpp>

namespace k3d
{

namespace ngui
{

namespace detail
{

const std::string plugin_factory_markup(k3d::iplugin_factory& Factory)
{
	std::string markup;

	if(k3d::iplugin_factory::EXPERIMENTAL == Factory.quality())
	{
		markup = k3d::string_cast(boost::format(_("<span color=\"blue\">%1% (Experimental)</span>")) % Factory.name());
	}
	else if(k3d::iplugin_factory::DEPRECATED == Factory.quality())
	{
		markup = k3d::string_cast(boost::format(_("<span color=\"red\" strikethrough=\"true\">%1%</span><span color=\"red\"> (Deprecated)</span>")) % Factory.name());
	}
	else
	{
		markup = Factory.name();
	}

	return markup;
}

} // namespace detail

Gtk::ImageMenuItem* create_menu_item(k3d::iplugin_factory& Factory)
{
	Gtk::Image* const image = new Gtk::Image(quiet_load_icon(Factory.name(), Gtk::ICON_SIZE_MENU));

	Gtk::ImageMenuItem* const menu_item =
		new Gtk::ImageMenuItem(
			*Gtk::manage(image),
			"",
			true)
		<< set_tooltip(Factory.short_description());

	get_label(*menu_item)->set_markup(detail::plugin_factory_markup(Factory));

	return menu_item;
}

} // namespace ngui

} // namespace k3d

