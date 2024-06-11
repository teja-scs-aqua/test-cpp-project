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
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/application_plugin_factory.h>
#include <k3dsdk/fstream.h>
#include <k3dsdk/ipath_property.h>
#include <k3dsdk/iselectable.h>
#include <k3dsdk/module.h>
#include <k3dsdk/ngui/button.h>
#include <k3dsdk/ngui/document_state.h>
#include <k3dsdk/ngui/icons.h>
#include <k3dsdk/ngui/image_toggle_button.h>
#include <k3dsdk/ngui/panel.h>
#include <k3dsdk/ngui/pipeline.h>
#include <k3dsdk/ngui/render.h>
#include <k3dsdk/ngui/scripting.h>
#include <k3dsdk/ngui/selection.h>
#include <k3dsdk/ngui/tool.h>
#include <k3dsdk/ngui/toolbar.h>
#include <k3dsdk/ngui/transform.h>
#include <k3dsdk/ngui/viewport.h>
#include <k3dsdk/ngui/widget_manip.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/result.h>
#include <k3dsdk/share.h>
#include <k3dsdk/utility_gl.h>

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/notebook.h>
#include <gtkmm/separatortoolitem.h>

#include <boost/assign/list_of.hpp>

// Temporary hack
using namespace k3d::ngui;

namespace module
{

namespace ngui
{

namespace toolbar
{

namespace detail
{

/// \deprecated Provides an implementation of k3d::toggle_button::imodel for visualizing the active tool
class builtin_tool_model :
	public image_toggle_button::imodel
{
public:
	builtin_tool_model(document_state& DocumentState, tool& Choice) :
		m_document_state(DocumentState),
		m_choice(Choice)
	{
	}

	const Glib::ustring label()
	{
		return m_choice.tool_type();
	}

	const k3d::bool_t value()
	{
		return &m_document_state.active_tool() == &m_choice;
	}

	void set_value(const k3d::bool_t Value)
	{
		m_document_state.set_active_tool(m_choice);
	}

	sigc::connection connect_changed_signal(const sigc::slot<void>& Slot)
	{
		return m_document_state.connect_active_tool_changed_signal(Slot);
	}

private:
	document_state& m_document_state;
	tool& m_choice;
};

/// Provides an implementation of k3d::toggle_button::imodel for visualizing the active tool
class plugin_tool_model :
	public image_toggle_button::imodel
{
public:
	plugin_tool_model(document_state& DocumentState, const k3d::string_t& Tool) :
		m_document_state(DocumentState),
		m_tool(Tool)
	{
	}

	const Glib::ustring label()
	{
		return m_tool;
	}

	const k3d::bool_t value()
	{
		return m_document_state.active_tool().tool_type() == m_tool;
	}

	void set_value(const k3d::bool_t Value)
	{
		k3d::ngui::tool* const tool = m_document_state.get_tool(m_tool);
		return_if_fail(tool);

		m_document_state.set_active_tool(*tool);
	}

	sigc::connection connect_changed_signal(const sigc::slot<void>& Slot)
	{
		return m_document_state.connect_active_tool_changed_signal(Slot);
	}

private:
	document_state& m_document_state;
	const k3d::string_t m_tool;
};

/// Provides an implementation of k3d::toggle_button::imodel for visualizing the current selection mode
class selection_mode_model :
	public toggle_button::imodel
{
public:
	selection_mode_model(k3d::idocument& Document, const selection::mode Mode, const Glib::ustring& Label) :
		m_document(Document),
		m_mode(Mode),
		m_label(Label)
	{
	}

	const Glib::ustring label()
	{
		return m_label;
	}

	const k3d::bool_t value()
	{
		return selection::state(m_document).current_mode() == m_mode;
	}

	void set_value(const k3d::bool_t Value)
	{
		selection::state(m_document).set_current_mode(m_mode);
	}

	sigc::connection connect_changed_signal(const sigc::slot<void>& Slot)
	{
		return selection::state(m_document).connect_current_mode_changed_signal(sigc::hide(Slot));
	}

private:
	k3d::idocument& m_document;
	const selection::mode m_mode;
	const Glib::ustring m_label;
};

/// Provides an implementation of k3d::toggle_button::imodel for visualizing the current keep selection mode
class keep_selection_model :
	public toggle_button::imodel
{
public:
	keep_selection_model(k3d::idocument& Document) :
		m_document(Document)
	{
	}

	const Glib::ustring label()
	{
		return _("Keep Selection");
	}

	const k3d::bool_t value()
	{
		return selection::state(m_document).keep_selection();
	}

	void set_value(const k3d::bool_t Value)
	{
		selection::state(m_document).set_keep_selection(Value);
	}

	sigc::connection connect_changed_signal(const sigc::slot<void>& Slot)
	{
		return selection::state(m_document).connect_keep_selection_changed_signal(sigc::hide(Slot));
	}

private:
	k3d::idocument& m_document;
};

/// Provides an implementation of k3d::toggle_button::imodel for visualizing the current convert selection mode
class convert_selection_model :
	public toggle_button::imodel
{
public:
	convert_selection_model(k3d::idocument& Document) :
		m_document(Document)
	{
	}

	const Glib::ustring label()
	{
		return _("Convert Selection");
	}

	const k3d::bool_t value()
	{
		return selection::state(m_document).convert_selection();
	}

	void set_value(const k3d::bool_t Value)
	{
		selection::state(m_document).set_convert_selection(Value);
	}

	sigc::connection connect_changed_signal(const sigc::slot<void>& Slot)
	{
		return selection::state(m_document).connect_convert_selection_changed_signal(sigc::hide(Slot));
	}

private:
	k3d::idocument& m_document;
};

/////////////////////////////////////////////////////////////////////////////
// implementation

struct implementation
{
	implementation(document_state& DocumentState) :
		m_document_state(DocumentState),
		m_toolbox(false, 0)
	{
		m_toolbox.get_accessible()->set_name("toolbox");
		k3d::ngui::toolbar::control* const main_toolbar = new k3d::ngui::toolbar::control();
		main_toolbar->get_accessible()->set_name("main_toolbar");
		main_toolbar->row(0).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::builtin_tool_model(m_document_state, m_document_state.selection_tool()), 0,
				load_icon("select_tool", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Select"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(0).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::builtin_tool_model(m_document_state, m_document_state.move_tool()), 0,
				load_icon("move_tool", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Move"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(0).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::builtin_tool_model(m_document_state, m_document_state.rotate_tool()), 0,
				load_icon("rotate_tool", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Rotate"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(0).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::builtin_tool_model(m_document_state, m_document_state.scale_tool()), 0,
				load_icon("scale_tool", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Scale"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		if(k3d::plugin::factory::lookup("NGUISnapTool"))
		{
			main_toolbar->row(0).pack_start(*Gtk::manage(
				new image_toggle_button::control(
					new detail::plugin_tool_model(m_document_state, "NGUISnapTool"), 0,
					load_icon("NGUISnapTool", Gtk::ICON_SIZE_SMALL_TOOLBAR))
				<< set_tooltip(_("Snap"))
				<< make_toolbar_button()
				), Gtk::PACK_SHRINK);
		}

		if(k3d::plugin::factory::lookup("NGUIParentTool"))
		{
			main_toolbar->row(0).pack_start(*Gtk::manage(
				new image_toggle_button::control(
					new detail::plugin_tool_model(m_document_state, "NGUIParentTool"), 0,
					load_icon("NGUIParentTool", Gtk::ICON_SIZE_SMALL_TOOLBAR))
				<< set_tooltip(_("Parent"))
				<< make_toolbar_button()
				), Gtk::PACK_SHRINK);
		}

		main_toolbar->row(0).pack_start(*Gtk::manage(
			button::create(
				*Gtk::manage(new Gtk::Image(load_icon("unparent", Gtk::ICON_SIZE_SMALL_TOOLBAR))))
			<< connect_button(sigc::mem_fun(*this, &implementation::on_unparent))
			<< set_tooltip(_("Unparent"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		if(k3d::plugin::factory::lookup("NGUIRenderRegionTool"))
		{
			main_toolbar->row(0).pack_start(*Gtk::manage(
				new image_toggle_button::control(
					new detail::plugin_tool_model(m_document_state, "NGUIRenderRegionTool"), 0,
					load_icon("NGUIRenderRegionTool", Gtk::ICON_SIZE_SMALL_TOOLBAR))
				<< set_tooltip(_("Render Region"))
				<< make_toolbar_button()), Gtk::PACK_SHRINK);
		}

		main_toolbar->row(0).pack_start(*Gtk::manage(
			button::create(
				*Gtk::manage(new Gtk::Image(load_icon("render_preview", Gtk::ICON_SIZE_SMALL_TOOLBAR))))
			<< connect_button(sigc::mem_fun(*this, &implementation::on_render_preview))
			<< set_tooltip(_("Render Preview"))
			<< make_toolbar_button()), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::selection_mode_model(m_document_state.document(), k3d::ngui::selection::NODE, _("Select Nodes")),
				&m_document_state.document().state_recorder(),
				load_icon("select_node", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Select Nodes"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::selection_mode_model(m_document_state.document(), k3d::ngui::selection::POINT, _("Select Points")),
				&m_document_state.document().state_recorder(),
				load_icon("select_vertex", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Select Points"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::selection_mode_model(m_document_state.document(), k3d::ngui::selection::EDGE, _("Select Polygon Edges")),
				&m_document_state.document().state_recorder(),
				load_icon("select_split_edge", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Select Polygon Edges"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::selection_mode_model(m_document_state.document(), k3d::ngui::selection::FACE, _("Select Polygons")),
				&m_document_state.document().state_recorder(),
				load_icon("select_face", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Select Polygons"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::selection_mode_model(m_document_state.document(), k3d::ngui::selection::CURVE, _("Select Curves")),
				&m_document_state.document().state_recorder(),
				load_icon("select_curve", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Select Curves"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::selection_mode_model(m_document_state.document(), k3d::ngui::selection::PATCH, _("Select Patches")),
				&m_document_state.document().state_recorder(),
				load_icon("select_patch", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Select Patches"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::selection_mode_model(m_document_state.document(), k3d::ngui::selection::SURFACE, _("Select Surface")),
				&m_document_state.document().state_recorder(),
				load_icon("select_uniform", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Select Surface"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::keep_selection_model(m_document_state.document()),
				0,
				load_icon("keep_selection", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Keep Selection"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		main_toolbar->row(1).pack_start(*Gtk::manage(
			new image_toggle_button::control(
				new detail::convert_selection_model(m_document_state.document()),
				0,
				load_icon("convert_selection", Gtk::ICON_SIZE_SMALL_TOOLBAR))
			<< set_tooltip(_("Convert Selection"))
			<< make_toolbar_button()
			), Gtk::PACK_SHRINK);

		Gtk::Notebook* const notebook = new Gtk::Notebook();

		const k3d::filesystem::path layout_path = k3d::share_path() / k3d::filesystem::generic_path("ngui/toolbar_layout.k3d");
		k3d::xml::element xml_layout;
		try
		{
			k3d::log() << info << "Loading toolbar layout from " << layout_path.native_console_string() << std::endl;
			k3d::filesystem::ifstream layout_stream(layout_path);
			layout_stream >> xml_layout;
			assert_warning(xml_layout.name == "k3dml");
		}
		catch(...)
		{
			k3d::log() << error << "Error loading toolbar layout from " << layout_path.native_console_string() << std::endl;
		}

		k3d::xml::element& xml_application = xml_layout.safe_element("application");
		k3d::xml::element& xml_user_interface = xml_application.safe_element("user_interface");
		k3d::xml::element& xml_toolbars = xml_user_interface.safe_element("toolbars");

		for(k3d::xml::element::elements_t::const_iterator xml_toolbar = xml_toolbars.children.begin(); xml_toolbar != xml_toolbars.children.end(); ++xml_toolbar)
		{
			if(xml_toolbar->name != "toolbar")
				continue;

			const std::string toolbar_name = k3d::xml::attribute_text(*xml_toolbar, "name");
			const std::string toolbar_label = k3d::xml::attribute_text(*xml_toolbar, "label");
			const std::string toolbar_description = k3d::xml::attribute_text(*xml_toolbar, "description");

			k3d::ngui::toolbar::control* const toolbar_widget = new k3d::ngui::toolbar::control();

			for(k3d::xml::element::elements_t::const_iterator xml_button = xml_toolbar->children.begin(); xml_button != xml_toolbar->children.end(); ++xml_button)
			{
				if(xml_button->name == "node")
				{
					const std::string name = k3d::xml::attribute_text(*xml_button, "name");
					k3d::iplugin_factory* const plugin_factory = k3d::plugin::factory::lookup(name);
					if(!plugin_factory)
						continue;

					const std::string node_name = "create_" + plugin_factory->name();
					Gtk::Image* const image = new Gtk::Image(load_icon(plugin_factory->name(), Gtk::ICON_SIZE_SMALL_TOOLBAR));

					Gtk::Button* const button =
						button::create(*manage(image))
						<< set_tooltip(plugin_factory->short_description())
						<< make_toolbar_button()
						<< connect_button(sigc::bind(sigc::mem_fun(*this, &implementation::on_create_node), plugin_factory))
						<< enable_dynamic_accelerators();

					toolbar_widget->row(0).pack_start(*Gtk::manage(button), Gtk::PACK_SHRINK);
				}
				else if(xml_button->name == "script")
				{
					const std::string name = k3d::xml::attribute_text(*xml_button, "name");
					const std::string label = k3d::xml::attribute_text(*xml_button, "label");
					const std::string description = k3d::xml::attribute_text(*xml_button, "description");

					const k3d::ipath_property::reference_t reference = k3d::xml::attribute_value<k3d::ipath_property::reference_t>(*xml_button, "reference", k3d::ipath_property::RELATIVE_REFERENCE);
					switch(reference)
					{
						case k3d::ipath_property::ABSOLUTE_REFERENCE:
							try
							{
								const k3d::filesystem::path absolute_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(k3d::xml::attribute_text(*xml_button, "absolute_path")));

								Gtk::Button* const button =
									new Gtk::Button(label)
									<< set_tooltip(description)
									<< connect_button(sigc::bind(sigc::mem_fun(*this, &implementation::on_run_external_script), absolute_path))
									<< make_toolbar_button()
									<< enable_dynamic_accelerators();

								toolbar_widget->row(0).pack_start(*Gtk::manage(button), Gtk::PACK_SHRINK);
							}
							catch(std::exception& e)
							{
								k3d::log() << error << e.what() << std::endl;
							}
							break;
						case k3d::ipath_property::RELATIVE_REFERENCE:
							try
							{
								k3d::filesystem::path root_path = k3d::share_path() / k3d::filesystem::generic_path("ngui");
								if(const k3d::xml::element* const xml_root = k3d::xml::find_element(*xml_button, "root"))
								{
									if(xml_root->text == "$K3D_SHARE_PATH")
										root_path = k3d::share_path();
									else
										root_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(xml_root->text));
								}

								const k3d::filesystem::path absolute_path = root_path / k3d::filesystem::native_path(k3d::ustring::from_utf8(k3d::xml::attribute_text(*xml_button, "relative_path")));

								Gtk::Button* const button =
									new Gtk::Button(label)
									<< set_tooltip(description)
									<< connect_button(sigc::bind(sigc::mem_fun(*this, &implementation::on_run_external_script), absolute_path))
									<< make_toolbar_button()
									<< enable_dynamic_accelerators();

								toolbar_widget->row(0).pack_start(*Gtk::manage(button), Gtk::PACK_SHRINK);
							}
							catch(std::exception& e)
							{
								k3d::log() << error << e.what() << std::endl;
							}
							break;
						case k3d::ipath_property::INLINE_REFERENCE:
							try
							{
								const std::string encoding = k3d::xml::attribute_text(*xml_button, "encoding");
								if(encoding != "text/plain")
									throw std::runtime_error("Toolbar script inline encoding must be text/plain");

								Gtk::Button* const button =
									new Gtk::Button(label)
									<< set_tooltip(description)
									<< connect_button(sigc::bind(sigc::mem_fun(*this, &implementation::on_run_inline_script), xml_button->text))
									<< make_toolbar_button()
									<< enable_dynamic_accelerators();

								toolbar_widget->row(0).pack_start(*Gtk::manage(button), Gtk::PACK_SHRINK);
							}
							catch(std::exception& e)
							{
								k3d::log() << error << e.what() << std::endl;
							}
							break;
					}
				}
				else if(xml_button->name == "separator")
				{
					toolbar_widget->row(0).pack_start(*Gtk::manage(new Gtk::SeparatorToolItem()), Gtk::PACK_SHRINK);
				}
			}

			notebook->append_page(*Gtk::manage(toolbar_widget), toolbar_label);
		}

		m_toolbox.pack_start(*Gtk::manage(main_toolbar), Gtk::PACK_SHRINK);
		m_toolbox.pack_start(*Gtk::manage(notebook), Gtk::PACK_EXPAND_WIDGET);
	}

	void on_create_node(k3d::iplugin_factory* const Factory)
	{
		return_if_fail(Factory);
		k3d::ngui::pipeline::create_node(m_document_state.document(), *Factory);
	}

	/// Unparents all selected nodes
	void on_unparent()
	{
		const k3d::nodes_t nodes = selection::state(m_document_state.document()).selected_nodes();
		if(nodes.empty())
			return;

		k3d::record_state_change_set change_set(m_document_state.document(), _("Unparent selected"), K3D_CHANGE_SET_CONTEXT);

		for(k3d::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
			k3d::ngui::unparent(**node);

		selection::state(m_document_state.document()).deselect_all();
		k3d::gl::redraw_all(m_document_state.document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_render_preview()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::icamera* camera = viewport_control->camera();
		if(!camera)
			camera = pick_camera(m_document_state);
		if(!camera)
			return;

		k3d::irender_camera_preview* render_engine = viewport_control->camera_preview_engine();
		if(!render_engine)
			render_engine = pick_camera_preview_render_engine(m_document_state);
		if(!render_engine)
			return;

		viewport_control->set_camera(camera);
		viewport_control->set_camera_preview_engine(render_engine);

		render(*camera, *render_engine);
	}

	void on_run_inline_script(const std::string Script)
	{
		k3d::script::code script(Script);

		k3d::iscript_engine::context context;
		context["document"] = &m_document_state.document();

		execute_script(script, "Inline Script", context);
	}

	void on_run_external_script(const k3d::filesystem::path Script)
	{
		k3d::iscript_engine::context context;
		context["document"] = &m_document_state.document();

		execute_script(Script, context);
	}

	/// Stores a reference to the owning document
	document_state& m_document_state;

	Gtk::HBox m_toolbox;
};

} // namespace detail

/////////////////////////////////////////////////////////////////////////////
// panel

class panel :
	public k3d::ngui::panel::control,
  public Gtk::VBox
{
	typedef Gtk::VBox base;

public:
	panel() :
		base(false, 0),
		m_implementation(0)
	{
		get_accessible()->set_name("vbox");
	}

	~panel()
	{
		delete m_implementation;
	}

	void initialize(document_state& DocumentState)
	{
		m_implementation = new detail::implementation(DocumentState);
		
		pack_start(m_implementation->m_toolbox, Gtk::PACK_SHRINK);
		show_all();
	}

	const k3d::string_t panel_type()
	{
		return get_factory().name();
	}

	sigc::connection connect_focus_signal(const sigc::slot<void>& Slot)
	{
		return sigc::connection();
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::application_plugin_factory<panel> factory(
			k3d::uuid(0xc4d044fe, 0x5043a104, 0xd97f0988, 0x19c053d8),
			"NGUIToolbarPanel",
			_("Provides the standard toolbar"),
			"NGUI Panel",
			k3d::iplugin_factory::STABLE,
			boost::assign::map_list_of("ngui:component-type", "panel")("ngui:panel-label", "Toolbar"));

		return factory;
	}

private:
	detail::implementation* m_implementation;
};

} // namespace toolbar

} // namespace ngui

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::ngui::toolbar::panel::get_factory());
K3D_MODULE_END

