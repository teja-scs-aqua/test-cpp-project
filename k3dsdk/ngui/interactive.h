#ifndef K3DSDK_NGUI_INTERACTIVE_H
#define K3DSDK_NGUI_INTERACTIVE_H

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

#include <k3dsdk/vectors.h>

namespace Gtk { class Widget; }

namespace k3d
{

namespace ngui
{

namespace interactive
{

/// Returns the current pointer screen coordinates
const k3d::point2 get_pointer();
	
/// Instantaneously warps the pointer from its current location to the given screen coordinates
void warp_pointer(const k3d::point2& Coords);
/// Instantaneously warps the pointer from its current location to the given widget coordinates
void warp_pointer(Gtk::Widget& Widget, const k3d::point2& Coords);
/// Instantaneously warps the pointer from its current location to the center of a widget
void warp_pointer(Gtk::Widget& Widget);

} // namespace interactive

} // namespace ngui

} // namespace k3d

#endif // !K3DSDK_NGUI_INTERACTIVE_H

