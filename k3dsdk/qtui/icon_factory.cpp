// K-3D
// Copyright (c) 1995-2010, Timothy M. Shead
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

#include <k3dsdk/inode.h>
#include <k3dsdk/iplugin_factory.h>
#include <k3dsdk/qtui/convert.h>
#include <k3dsdk/qtui/icon_factory.h>
#include <k3dsdk/share.h>
#include <QIcon>
#include <map>

namespace k3d
{

namespace qtui
{

/// Caches icons that have already been loaded into memory
static std::map<iplugin_factory*, QIcon> plugin_icons;

QIcon icon_factory::create(inode& Node)
{
	return create(Node.factory());
}

QIcon icon_factory::create(iplugin_factory& Plugin)
{
	if(!plugin_icons.count(&Plugin))
	{
		plugin_icons.insert(std::make_pair(&Plugin,
			QIcon(convert<QString>((share_path() / filesystem::generic_path("qtui/icons/" + Plugin.name() + ".svg")).native_filesystem_string()))));
	}

	return plugin_icons[&Plugin];
}

} // namespace qtui

} // namespace k3d

