// K-3D
// Copyright (c) 1995-2009, Timothy M. Shead
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
	\author Timothy M. Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/hints.h>
#include <k3dsdk/node.h>
#include <k3dsdk/value_demand_storage.h>

#include <boost/format.hpp>

namespace module
{

namespace scalar
{

class double_to_string :
	public k3d::node
{
	typedef k3d::node base;
public:
	double_to_string(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_input(init_owner(*this) + init_name("input") + init_label(_("Input")) + init_description(_("Float value")) + init_value(0.0)),
		m_format(init_owner(*this) + init_name("format") + init_label(_("Format")) + init_description(_("printf()-style format string.")) + init_value(std::string("%f"))),
		m_output(init_owner(*this) + init_name("output") + init_label(_("Output text")) + init_description(_("Output string.")) + init_value(k3d::string_t()))
	{
		m_input.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(m_output.make_slot()));
		m_format.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(m_output.make_slot()));

		m_output.set_update_slot(sigc::mem_fun(*this, &double_to_string::execute));
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<double_to_string > factory(
			k3d::uuid(0x3af7d777, 0x691d41b0, 0xaa801d59, 0x7ee4180e),
			"DoubleToString",
			_("Converts a double to a string using printf() style double_to_stringting"),
			"Double String",
			k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_input;
	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_format;
	k3d_data(k3d::string_t, immutable_name, change_signal, no_undo, value_demand_storage, no_constraint, read_only_property, no_serialization) m_output;

	void execute(const std::vector<k3d::ihint*>& Hints, k3d::string_t& Output)
	{
		try
		{
			Output = (boost::format(m_format.pipeline_value()) % m_input.pipeline_value()).str();
		}
		catch(std::exception& e)
		{
			k3d::log() << error << e.what() << std::endl;
			Output = k3d::string_t();
		}
	}

};

k3d::iplugin_factory& double_to_string_factory()
{
	return double_to_string::get_factory();
}

} //namespace scalar

} // namespace module

