// K-3D
// Copyright (c) 1995-2007, Timothy M. Shead
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
	\author Romain Behar (romainbehar@yahoo.com)
	\author Timothy M. Shead (tshead@k-3d.com)
*/

#include "utility.h"

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/mesh_painter_gl.h>
#include <k3dsdk/options.h>
#include <k3dsdk/painter_render_state_gl.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/share.h>

#include <FTGL/ftgl.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace opengl
{

namespace painters
{

namespace detail
{

typedef std::vector<k3d::string_t> strings_t;

/// Converts the named array with the given name to an array of strings
template<typename ArrayT>
void named_array_to_strings(const k3d::mesh::table_t& Arrays, const k3d::string_t& ArrayName, strings_t& StringArray)
{
	const ArrayT* array = Arrays.lookup<ArrayT>(ArrayName);
	if (!array)
		return;
	
	const k3d::uint_t array_size = array->size();
	StringArray.resize(array_size);
	for(k3d::uint_t i = 0; i != array_size; ++i)
	{
		StringArray[i] = k3d::string_cast(array->at(i));
	}
}

}

/////////////////////////////////////////////////////////////////////////////
// vertex_data_painter

class vertex_data_painter :
	public k3d::gl::mesh_painter
{
	typedef k3d::gl::mesh_painter base;

public:
	vertex_data_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_font_path(init_owner(*this) + init_name("font") + init_label(_("Font")) + init_description(_("Font path")) + init_value(k3d::share_path() / k3d::filesystem::generic_path("fonts/VeraBd.ttf")) + init_path_mode(k3d::ipath_property::READ) + init_path_type(k3d::options::path::fonts())),
		m_font_size(init_owner(*this) + init_name("font_size") + init_label(_("Font Size")) + init_description(_("Font size.")) + init_value(14.0)),
		m_antialias(init_owner(*this) + init_name("antialias") + init_label(_("Font Antialiasing")) + init_description(_("Render antialiased text.")) + init_value(true)),
		m_color(init_owner(*this) + init_name("color") + init_label(_("Color")) + init_description(_("Color of the numbers")) + init_value(k3d::color(0.5, 0, 0))),
		m_array_name(init_owner(*this) + init_name("array_name") + init_label(_("Array Name")) + init_description(_("Specifies the array to draw")) + init_value(std::string("Cs")))
	{
		m_font_path.changed_signal().connect(sigc::mem_fun(*this, &vertex_data_painter::on_font_changed));
		m_font_size.changed_signal().connect(sigc::mem_fun(*this, &vertex_data_painter::on_font_changed));
		m_antialias.changed_signal().connect(sigc::mem_fun(*this, &vertex_data_painter::on_font_changed));
		m_color.changed_signal().connect(make_async_redraw_slot());
		m_array_name.changed_signal().connect(make_async_redraw_slot());
	}

	void on_font_changed(k3d::ihint*)
	{
		m_font.reset();
		async_redraw(0);
	}
	
	void draw(const k3d::typed_array<k3d::point3>& Points, const k3d::color& Color, FTFont& Font, const detail::strings_t StringArray)
	{
		k3d::gl::color3d(Color);

		const size_t point_begin = 0;
		const size_t point_end = point_begin + Points.size();
		for(size_t point = point_begin; point != point_end; ++point)
		{
			const k3d::point3 position = Points[point];
			glRasterPos3d(position[0], position[1], position[2]);
			if(point < StringArray.size())
				Font.Render(StringArray[point].c_str());
		}
	}

	void on_paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		if(!Mesh.points)
			return;

		if(!m_font)
		{
			if(m_antialias.pipeline_value())
				m_font.reset(new FTPixmapFont(m_font_path.pipeline_value().native_filesystem_string().c_str()));
			else
				m_font.reset(new FTBitmapFont(m_font_path.pipeline_value().native_filesystem_string().c_str()));

			m_font->FaceSize(static_cast<unsigned int>(m_font_size.pipeline_value()));
			m_font->UseDisplayList(true);
			if(m_font->Error())
			{
				k3d::log() << error << "error initializing font" << std::endl;
				return;
			}
		}

		const k3d::mesh::points_t& points = *Mesh.points;

		k3d::gl::store_attributes attributes;
		glDisable(GL_LIGHTING);
		
		// Try some different types for the array to print
		const k3d::string_t array_name = m_array_name.pipeline_value();
		detail::strings_t string_array;
		detail::named_array_to_strings<k3d::mesh::colors_t>(Mesh.point_attributes, array_name, string_array);
		detail::named_array_to_strings<k3d::mesh::normals_t>(Mesh.point_attributes, array_name, string_array);
		detail::named_array_to_strings<k3d::mesh::indices_t>(Mesh.point_attributes, array_name, string_array);
		detail::named_array_to_strings<k3d::mesh::weights_t>(Mesh.point_attributes, array_name, string_array);
		detail::named_array_to_strings<k3d::mesh::points_t>(Mesh.point_attributes, array_name, string_array);
		draw(points, m_color.pipeline_value(), *m_font, string_array);
	}
	
	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<vertex_data_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
			k3d::uuid(0x40dd537a, 0xe84d0fd8, 0x416941a3, 0x719fab2d),
			"OpenGLVertexDataPainter",
			_("Prints vertex data near each corresponding vertex"),
			"OpenGL Painter",
			k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}

private:
	k3d_data(k3d::filesystem::path, immutable_name, change_signal, with_undo, local_storage, no_constraint, path_property, path_serialization) m_font_path;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_font_size;
	k3d_data(k3d::bool_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_antialias;
	k3d_data(k3d::color, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_color;
	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_array_name;

	boost::scoped_ptr<FTFont> m_font;
};

/////////////////////////////////////////////////////////////////////////////
// vertex_data_painter_factory

k3d::iplugin_factory& vertex_data_painter_factory()
{
	return vertex_data_painter::get_factory();
}

} // namespace painters

} // namespace opengl

} // namespace module

