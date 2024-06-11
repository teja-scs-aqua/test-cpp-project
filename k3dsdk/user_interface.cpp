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
		\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/log.h>
#include <k3dsdk/result.h>
#include <k3dsdk/user_interface_init.h>
#include <k3dsdk/user_interface.h>

namespace k3d
{

namespace detail
{

/// Stores the global user_interface object
iuser_interface* g_user_interface = 0;
	
} // namespace detail
	
void set_user_interface(iuser_interface& UserInterface)
{
	return_if_fail(!detail::g_user_interface);
	detail::g_user_interface = &UserInterface;
}

iuser_interface& user_interface()
{
	assert_critical(detail::g_user_interface);
	return *detail::g_user_interface;
}

} // namespace k3d

