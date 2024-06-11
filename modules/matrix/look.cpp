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
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/algebra.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/transformable.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/node.h>
#include <k3dsdk/vectors.h>

namespace module
{

namespace matrix
{

/////////////////////////////////////////////////////////////////////////////
// look

class look :
	public k3d::transformable<k3d::node >
{
	typedef k3d::transformable<k3d::node > base;

public:
	look(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_target(init_owner(*this) + init_name("target_matrix") + init_label(_("Target matrix")) + init_description(_("Target matrix")) + init_value(k3d::identity3()))
	{
		m_target.changed_signal().connect(k3d::hint::converter<
			k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_update_matrix_slot()));
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<look,
			k3d::interface_list<k3d::imatrix_source,
			k3d::interface_list<k3d::imatrix_sink > > > factory(
				k3d::uuid(0x4e2a30f5, 0x6d7d47ad, 0x943ccd36, 0x4b305b55),
				"MatrixLook",
				_("Creates a transformation matrix that looks from one to another."),
				"Matrix",
				k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	void on_update_matrix(const k3d::matrix4& Input, k3d::matrix4& Output)
	{
		const k3d::matrix4 target_matrix = m_target.pipeline_value();

		const k3d::point3 from = Input * k3d::point3(0, 0, 0);
		const k3d::point3 to = target_matrix * k3d::point3(0, 0, 0);
		const k3d::vector3 spherical = k3d::spherical(to - from);

		Output = Input * rotate3(k3d::quaternion(k3d::euler_angles(0, -spherical[2], spherical[1], k3d::euler_angles::ZXYstatic)));
	}

	k3d_data(k3d::matrix4, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_target;
};

/////////////////////////////////////////////////////////////////////////////
// look_factory

k3d::iplugin_factory& look_factory()
{
	return look::get_factory();
}

} // namespace matrix

} // namespace module

