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
	\author Wladyslaw Strugala (fizws@julia.univ.gda.pl)
	\author Romain Behar (romainbehar@yahoo.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/file_helpers.h>
#include <k3dsdk/gzstream.h>
#include <k3dsdk/imaterial.h>
#include <k3dsdk/material_sink.h>
#include <k3dsdk/mesh_reader.h>
#include <k3dsdk/node.h>
#include <k3dsdk/persistent_lookup.h>
#include <k3dsdk/serialization_xml.h>

namespace module
{

namespace k3d_io
{

class set_default_material
{
public:
	set_default_material(k3d::imaterial* Material) : m_material(Material) {}

	void operator()(const k3d::string_t&, const k3d::table&, const k3d::string_t& ArrayName, k3d::pipeline_data<k3d::array>& Array)
	{
		k3d::mesh::materials_t* const materials = dynamic_cast<k3d::mesh::materials_t*>(&Array.writable());
		if(!materials)
		{
			return;
		}

		materials->assign(materials->size(), m_material);
	}

private:
	k3d::imaterial* m_material;
};

/////////////////////////////////////////////////////////////////////////////
// mesh_reader

class mesh_reader :
	public k3d::material_sink<k3d::mesh_reader<k3d::node > >
{
	typedef k3d::material_sink<k3d::mesh_reader<k3d::node > > base;

public:
	mesh_reader(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document)
	{
		m_material.changed_signal().connect(k3d::hint::converter<
				k3d::hint::convert<k3d::hint::any, k3d::hint::none> >(make_reload_mesh_slot()));
	}

	void on_load_mesh(const k3d::filesystem::path& Path, k3d::mesh& Output)
	{
		Output = k3d::mesh();
		try
		{
			k3d::filesystem::igzstream file(Path);
			if(!file)
			{
				k3d::log() << error << k3d_file_reference << ": error opening [" << Path.native_console_string() << "]" << std::endl;
				return;
			}

			k3d::xml::element xml_document("k3dml");
			file >> xml_document;

			const k3d::filesystem::path root_path = Path.branch_path();
			k3d::persistent_lookup lookup;
			k3d::ipersistent::load_context context(root_path, lookup);

			if(k3d::xml::element* const xml_mesh_arrays = k3d::xml::find_element(xml_document, "mesh_arrays"))
				k3d::xml::load(Output, *xml_mesh_arrays, context);
		}
		catch(std::exception& e)
		{
			k3d::log() << error << k3d_file_reference << ": error opening [" << Path.native_console_string() << "]" << std::endl;
			k3d::log() << error << e.what() << std::endl;
			Output = k3d::mesh();
			return;
		}
		// Assign a default material
		k3d::mesh::visit_arrays(Output, set_default_material(m_material.pipeline_value()));
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<mesh_reader,
			k3d::interface_list<k3d::imesh_source,
			k3d::interface_list<k3d::imesh_storage> > > factory(
				k3d::uuid(0x7dd19499, 0x965e4c06, 0xb970f545, 0x0e47855d),
				"K3DMeshReader",
				_("Mesh reader that loads external K-3D XML (.k3d) files"),
				"MeshReader");

		return factory;
	}
};

k3d::iplugin_factory& mesh_reader_factory()
{
	return mesh_reader::get_factory();
}

} // namespace k3d_io

} // namespace module

