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

#include <k3dsdk/ngui/null_viewport_input_model.h>

namespace k3d
{

namespace ngui
{

/////////////////////////////////////////////////////////////////////////////
// null_viewport_input_model

void null_viewport_input_model::button_press_event(viewport::control& Viewport, GdkEventButton* Event)
{
}

void null_viewport_input_model::button_release_event(viewport::control& Viewport, GdkEventButton* Event)
{
}

void null_viewport_input_model::motion_notify_event(viewport::control& Viewport, GdkEventMotion* Event)
{
}

void null_viewport_input_model::scroll_event(viewport::control& Viewport, GdkEventScroll* Event)
{
}

void null_viewport_input_model::key_press_event(viewport::control& Viewport, GdkEventKey* Event)
{
}

void null_viewport_input_model::key_release_event(viewport::control& Viewport, GdkEventKey* Event)
{

}

} // namespace ngui

} // namespace k3d

