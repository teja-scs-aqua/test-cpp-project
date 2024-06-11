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
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/algebra.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/node.h>
#include <k3dsdk/resource/resource.h>
#include <k3dsdk/scripted_node.h>
#include <k3dsdk/transformable.h>

namespace module
{

namespace scripting
{

/////////////////////////////////////////////////////////////////////////////
// matrix_modifier_script

class matrix_modifier_script :
	public k3d::scripted_node<k3d::transformable<k3d::node > >
{
	typedef k3d::scripted_node<k3d::transformable<k3d::node > > base;

public:
	matrix_modifier_script(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document)
	{
		set_script(k3d::resource::get_string("/module/scripting/matrix_modifier_script.py"));
		
		connect_script_changed_signal(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_update_matrix_slot()));
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<matrix_modifier_script,
			k3d::interface_list<k3d::imatrix_sink, k3d::interface_list<k3d::imatrix_source> > > factory(
			k3d::uuid(0xacafcc85, 0xa0bf4d69, 0x99592c4f, 0x7cf9b35c),
			"MatrixModifierScript",
			_("Matrix modifier that uses a script to modify a transformation matrix"),
			"Script Matrix",
			k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	void on_update_matrix(const k3d::matrix4& Input, k3d::matrix4& Output)
	{		
		// Create a new output matrix, ready for modification by the script ...
		Output = Input;

		k3d::iscript_engine::context context;
		context["document"] = &document();
		context["node"] = static_cast<k3d::inode*>(this);
		context["input"] = Input;
		context["output"] = Output;

		execute_script(context);

		return_if_fail(context["output"].type() == typeid(k3d::matrix4));
		Output = boost::any_cast<k3d::matrix4>(context["output"]);
	}
};

/////////////////////////////////////////////////////////////////////////////
// matrix_modifier_script_factory

k3d::iplugin_factory& matrix_modifier_script_factory()
{
	return matrix_modifier_script::get_factory();
}

} // namespace scripting

} // namespace module


