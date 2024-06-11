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

#include <k3d-i18n-config.h>
#include <k3dsdk/application_plugin_factory.h>
#include <k3dsdk/log.h>
#include <k3dsdk/module.h>
#include <k3dsdk/qtui/mode.h>

#include <boost/assign/list_of.hpp>

namespace module
{

namespace qtui
{

namespace def
{

/////////////////////////////////////////////////////////////////////////////
// mode

/// Sets-up a default mode for use when no other mode is in-effect.
class mode :
	public k3d::qtui::mode,
	public k3d::iunknown
{
	typedef k3d::qtui::mode base;

public:
	mode()
	{
	}

	void enable(k3d::idocument& Document, QGraphicsScene& Scene)
	{
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::application_plugin_factory<mode> factory(
			k3d::uuid(0x6b5cbfd7, 0x8742246c, 0x5345988d, 0x2e14ecf7),
			"QTUIDefaultMode",
			_("Provides default behavior when no other mode is in-use."),
			"QTUI Mode",
			k3d::iplugin_factory::EXPERIMENTAL,
			boost::assign::map_list_of("qtui:component-type", "mode"));

		return factory;
	}
};

} // namespace def

} // namespace qtui

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::qtui::def::mode::get_factory());
K3D_MODULE_END

