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
	\author Tim Shead (tshead@k-3d.com)
*/

#include "common.h"

#include <k3d-i18n-config.h>
#include <k3dsdk/basic_math.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/metadata.h>
#include <k3dsdk/node.h>
#include <k3dsdk/options.h>
#include <k3dsdk/renderable_gl.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/share.h>
#include <k3dsdk/transformable.h>
#include <k3dsdk/vectors.h>

#include <FTGL/ftgl.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace annotation
{

/////////////////////////////////////////////////////////////////////////////
// annotation_3d

class annotation_3d :
	public k3d::gl::renderable<k3d::transformable<k3d::node > >
{
	typedef k3d::gl::renderable<k3d::transformable<k3d::node > > base;

public:
	annotation_3d(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_font_path(init_owner(*this) + init_name("font") + init_label(_("Font")) + init_description(_("Font path")) + init_value(k3d::share_path() / k3d::filesystem::generic_path("fonts/VeraBd.ttf")) + init_path_mode(k3d::ipath_property::READ) + init_path_type(k3d::options::path::fonts())),
		m_font_size(init_owner(*this) + init_name("font_size") + init_label(_("Font Size")) + init_description(_("Font size.")) + init_value(14.0)),
		m_line_width(init_owner(*this) + init_name("line_width") + init_label(_("Line Width")) + init_description(_("Maximum width of a single line of text..")) + init_value(200.0)),
		m_line_spacing(init_owner(*this) + init_name("line_spacing") + init_label(_("Line Spacing")) + init_description(_("Controls the spacing between lines of text.")) + init_value(1.0)),
		m_alignment(init_owner(*this) + init_name("alignment") + init_label(_("Alignment")) + init_description(_("Controls the alignment of adjacent lines of text.")) + init_value(LEFT) + init_values(alignment_values())),
		m_text(init_owner(*this) + init_name("text") + init_label(_("Text")) + init_description(_("Annotation text")) + init_value(k3d::string_t(_("Annotation")))),
		m_color(init_owner(*this) + init_name("color") + init_label(_("Color")) + init_description(_("Annotation color")) + init_value(k3d::color(0, 0, 0)))
	{
		m_text.set_metadata_value("k3d:property-type", "k3d:multi-line-text");

		m_font_path.changed_signal().connect(make_async_redraw_slot());
		m_font_size.changed_signal().connect(make_async_redraw_slot());
		m_line_width.changed_signal().connect(make_async_redraw_slot());
		m_line_spacing.changed_signal().connect(make_async_redraw_slot());
		m_alignment.changed_signal().connect(make_async_redraw_slot());
		m_text.changed_signal().connect(make_async_redraw_slot());
		m_color.changed_signal().connect(make_async_redraw_slot());
		m_input_matrix.changed_signal().connect(make_async_redraw_slot());
	}

	void on_gl_draw(const k3d::gl::render_state& State)
	{
		k3d::gl::color3d(State.node_selection ? k3d::color(1, 1, 1) : m_color.pipeline_value());
		draw(State);
	}

	void on_gl_select(const k3d::gl::render_state& State, const k3d::gl::selection_state& SelectState)
	{
		k3d::gl::push_selection_token(this);
		draw(State);
		k3d::gl::pop_selection_token();
	}

	void draw(const k3d::gl::render_state& State)
	{
		FTGLPolygonFont font(m_font_path.pipeline_value().native_filesystem_string().c_str());
		if(font.Error())
		{
			k3d::log() << error << "error initializing font" << std::endl;
			return;
		}
		font.FaceSize(static_cast<unsigned int>(m_font_size.pipeline_value()));

		FTSimpleLayout layout;
		layout.SetFont(&font);
		layout.SetLineLength(m_line_width.pipeline_value());
		layout.SetLineSpacing(m_line_spacing.pipeline_value());

		switch(m_alignment.pipeline_value())
		{
			case LEFT:
				layout.SetAlignment(FTGL::ALIGN_LEFT);
				break;
			case CENTER:
				layout.SetAlignment(FTGL::ALIGN_CENTER);
				break;
			case RIGHT:
				layout.SetAlignment(FTGL::ALIGN_RIGHT);
				break;
			case JUSTIFY:
				layout.SetAlignment(FTGL::ALIGN_JUSTIFY);
				break;
		}

		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_1D);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);

		glRasterPos3d(0, 0, 0);
		layout.Render(m_text.pipeline_value().c_str());
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<annotation_3d,
				k3d::interface_list<k3d::imatrix_source,
				k3d::interface_list<k3d::imatrix_sink > > >factory(
			k3d::uuid(0xc507e5ea, 0x8f425637, 0x85b2829d, 0x7eb1c4c4),
			"Annotation3D",
			_("Displays text annotations in the 3D document, primarily for documentation / tutorials"),
			"Annotation",
			k3d::iplugin_factory::STABLE);

		return factory;
	}

private:
	k3d_data(k3d::filesystem::path, immutable_name, change_signal, with_undo, local_storage, no_constraint, path_property, path_serialization) m_font_path;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_font_size;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_line_width;
	k3d_data(k3d::double_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_line_spacing;
	k3d_data(alignment_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, enumeration_property, with_serialization) m_alignment;
	k3d::metadata::property<k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization)> m_text;
	k3d_data(k3d::color, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_color;
};

/////////////////////////////////////////////////////////////////////////////
// annotation_3d_factory

k3d::iplugin_factory& annotation_3d_factory()
{
	return annotation_3d::get_factory();
}

} // namespace annotation

} // namespace module

