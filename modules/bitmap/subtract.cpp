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
	\author Anders Dahnielson (anders@dahnielson.com)
*/

#include "simple_modifier.h"

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>

namespace module
{

namespace bitmap
{

/////////////////////////////////////////////////////////////////////////////
// subtract

class subtract :
	public simple_modifier
{
	typedef simple_modifier base;

public:
	subtract(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_value(init_owner(*this) + init_name("value") + init_label(_("Subtract value")) + init_description(_("Subtract value to each pixel color component")) + init_value(0.0))
	{
		m_value.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::bitmap_pixels_changed> >(make_update_bitmap_slot()));
	}

	struct functor
	{
		functor(const double Value) :
			value(Value)
		{
		}

		k3d::pixel operator()(const k3d::pixel& Input) const
		{
			return k3d::pixel(
				std::max(0.0, boost::gil::get_color(Input, boost::gil::red_t()) - value),
				std::max(0.0, boost::gil::get_color(Input, boost::gil::green_t()) - value),
				std::max(0.0, boost::gil::get_color(Input, boost::gil::blue_t()) - value),
				boost::gil::get_color(Input, boost::gil::alpha_t()));
		}

		const double value;
	};

	void on_assign_pixels(const k3d::bitmap& Input, k3d::bitmap& Output)
	{
		boost::gil::transform_pixels(const_view(Input), view(Output), functor(m_value.pipeline_value()));
	}


	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<subtract,
			k3d::interface_list<k3d::ibitmap_source,
			k3d::interface_list<k3d::ibitmap_sink> > > factory(
				k3d::uuid(0x51c8f52f, 0x53834926, 0x865e3592, 0xf0d09510),
				"BitmapSubtract",
				_("Subtract value from each pixel"),
				"Bitmap",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_value;
	double m_value_cache;
};

/////////////////////////////////////////////////////////////////////////////
// subtract_factory

k3d::iplugin_factory& subtract_factory()
{
	return subtract::get_factory();
}

} // namespace bitmap

} // namespace module


