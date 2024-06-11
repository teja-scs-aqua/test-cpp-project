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
// gamma

class gamma :
	public simple_modifier
{
	typedef simple_modifier base;

public:
	gamma(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_gamma(init_owner(*this) + init_name("gamma") + init_label(_("Gamma Value")) + init_description(_("Apply gamma value to each pixel.")) + init_value(1.0))
	{
		m_gamma.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::bitmap_pixels_changed> >(make_update_bitmap_slot()));
	}

	struct functor
	{
		functor(const double Gamma) :
			gamma(Gamma ? 1.0 / Gamma : 1.0)
		{
		}

		k3d::pixel operator()(const k3d::pixel& Input)
		{
			return k3d::pixel(
				std::pow(static_cast<double>(boost::gil::get_color(Input, boost::gil::red_t())), gamma),
				std::pow(static_cast<double>(boost::gil::get_color(Input, boost::gil::green_t())), gamma),
				std::pow(static_cast<double>(boost::gil::get_color(Input, boost::gil::blue_t())), gamma),
				boost::gil::get_color(Input, boost::gil::alpha_t()));
		}

		const double gamma;
	};

	void on_assign_pixels(const k3d::bitmap& Input, k3d::bitmap& Output)
	{
		boost::gil::transform_pixels(const_view(Input), view(Output), functor(m_gamma.pipeline_value()));
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<gamma,
			k3d::interface_list<k3d::ibitmap_source,
			k3d::interface_list<k3d::ibitmap_sink> > > factory(
				k3d::uuid(0xa2ff08c9, 0x96e54063, 0x907ad507, 0xec36dd1c),
				"BitmapGamma",
				_("Apply gamma value to each pixel"),
				"Bitmap",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_gamma;
};

/////////////////////////////////////////////////////////////////////////////
// gamma_factory

k3d::iplugin_factory& gamma_factory()
{
	return gamma::get_factory();
}

} // namespace bitmap

} // namespace module

