#ifndef K3DSDK_NGUI_MENUS_H
#define K3DSDK_NGUI_MENUS_H

// K-3D
// Copyright (c) 1995-2004, Timothy M. Shead
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

/** \author Timothy M. Shead (tshead@k-3d.com)
*/

#include <string>

namespace k3d { class iplugin_factory; }
namespace Gtk { class ImageMenuItem; }

namespace k3d
{

namespace ngui
{

/// Creates a menu item based on a plugin type
Gtk::ImageMenuItem* create_menu_item(k3d::iplugin_factory& Factory);

} // namespace ngui

} // namespace k3d

#endif // !K3DSDK_NGUI_MENUS_H
