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
	\author Romain Behar (romainbehar@yahoo.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/algebra.h>
#include <k3dsdk/application.h>
#include <k3dsdk/axis.h>
#include <k3dsdk/basic_math.h>
#include <k3dsdk/batch_mode.h>
#include <k3dsdk/classes.h>
#include <k3dsdk/fstream.h>
#include <k3dsdk/gzstream.h>
#include <k3dsdk/iapplication.h>
#include <k3dsdk/icamera.h>
#include <k3dsdk/idocument_exporter.h>
#include <k3dsdk/idocument.h>
#include <k3dsdk/idocument_importer.h>
#include <k3dsdk/idocument_plugin_factory.h>
#include <k3dsdk/idocument_sink.h>
#include <k3dsdk/imesh_sink.h>
#include <k3dsdk/imesh_source.h>
#include <k3dsdk/iparentable.h>
#include <k3dsdk/iplugin_factory_collection.h>
#include <k3dsdk/iplugin_factory.h>
#include <k3dsdk/iscripted_action.h>
#include <k3dsdk/iselectable.h>
#include <k3dsdk/itime_sink.h>
#include <k3dsdk/imatrix_sink.h>
#include <k3dsdk/imatrix_source.h>
#include <k3dsdk/iuser_interface.h>
#include <k3dsdk/mime_types.h>
#include <k3dsdk/ngui/application_state.h>
#include <k3dsdk/ngui/check_menu_item.h>
#include <k3dsdk/ngui/pipeline.h>
#include <k3dsdk/ngui/document.h>
#include <k3dsdk/ngui/document_state.h>
#include <k3dsdk/ngui/file_chooser_dialog.h>
#include <k3dsdk/ngui/icons.h>
#include <k3dsdk/ngui/image_toggle_button.h>
#include <k3dsdk/ngui/main_document_window.h>
#include <k3dsdk/ngui/menus.h>
#include <k3dsdk/ngui/merge_nodes.h>
#include <k3dsdk/ngui/messages.h>
#include <k3dsdk/ngui/modifiers.h>
#include <k3dsdk/ngui/node.h>
#include <k3dsdk/ngui/panel_frame.h>
#include <k3dsdk/ngui/panel_mediator.h>
#include <k3dsdk/ngui/render.h>
#include <k3dsdk/ngui/savable_document_window.h>
#include <k3dsdk/ngui/scripting.h>
#include <k3dsdk/ngui/selection.h>
#include <k3dsdk/ngui/target.h>
#include <k3dsdk/ngui/toolbar.h>
#include <k3dsdk/ngui/transform.h>
#include <k3dsdk/ngui/undo_utility.h>
#include <k3dsdk/ngui/uri.h>
#include <k3dsdk/ngui/utility.h>
#include <k3dsdk/ngui/viewport.h>
#include <k3dsdk/ngui/widget_manip.h>
#include <k3dsdk/node.h>
#include <k3dsdk/nodes.h>
#include <k3dsdk/options.h>
#include <k3dsdk/persistent_lookup.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/property.h>
#include <k3dsdk/property_collection.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/share.h>
#include <k3dsdk/state_change_set.h>
#include <k3dsdk/string_cast.h>
#include <k3dsdk/system.h>
#include <k3dsdk/transform.h>
#include <k3dsdk/user_property.h>
#include <k3dsdk/utility_gl.h>
#include <k3dsdk/xml.h>
#include <k3d-version-config.h>

#include <gdkmm/cursor.h>
#include <gtkmm/accelmap.h>
#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/frame.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/handlebox.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/menubar.h>
#include <gtkmm/notebook.h>
#include <gtkmm/paned.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/stock.h>
#include <gtkmm/textview.h>
#include <gtkmm/treeview.h>

#include <sigc++/retype_return.h>

#include <boost/scoped_ptr.hpp>

#include <sstream>

#include <gtk/gtkbutton.h>

namespace k3d
{

namespace ngui
{

namespace detail
{

/////////////////////////////////////////////////////////////////////////////
// select: selecting nodes using std::for_each

struct select
{
	select(idocument& Document) : document(Document)
	{
	}

	void operator()(k3d::inode* Node)
	{
		if(Node)
			selection::state(document).select(*Node);
	}
private:
	idocument& document;
};

/////////////////////////////////////////////////////////////////////////////
// get_panel_frames

void get_panel_frames(Gtk::Widget& Parent, panel_frame::controls& Results)
{
	panel_frame::control* const panel = dynamic_cast<panel_frame::control*>(&Parent);
	if(panel)
	{
		Results.push_back(panel);
		return;
	}

	Gtk::Container* const container = dynamic_cast<Gtk::Container*>(&Parent);
	if(container)
	{
		std::vector<Gtk::Widget*> children = container->get_children();
		for(std::vector<Gtk::Widget*>::iterator child = children.begin(); child != children.end(); ++child)
			get_panel_frames(**child, Results);
	}
}

panel_frame::controls get_panel_frames(Gtk::Widget& Parent)
{
	panel_frame::controls results;
	get_panel_frames(Parent, results);

	return results;
}

/////////////////////////////////////////////////////////////////////////////
// ui_layout_path

const k3d::filesystem::path ui_layout_path()
{
	return k3d::system::get_home_directory() / k3d::filesystem::generic_path(".k3d/ui_layout");
}

} // namespace detail

/////////////////////////////////////////////////////////////////////////////
// main_document_window

class main_document_window :
	public savable_document_window,
	public k3d::property_collection,
	public k3d::ipersistent
{
	typedef savable_document_window base;

public:
	main_document_window(document_state& DocumentState) :
		k3d::property_collection(),
		m_maximize_panel(init_name("maximize_panel") + init_label(_("Maximize active panel")) + init_description(_("Maximize active panel (make it the only visible one)")) + init_value(false)),
		m_hide_unpinned_panels(init_name("hide_unpinned_panels") + init_label(_("Hide unpinned panels")) + init_description(_("Hide/show unpinned panels in main document window")) + init_value(false)),
		m_fullscreen(init_name("fullscreen") + init_label(_("Fullscreen")) + init_description(_("Fullscreen mode for main document window")) + init_value(false)),
		m_focus_panel(0),
		m_focus_viewport_panel(0),
		m_layout_maximize_panel(0),
		m_layout_hide_unpinned(0),
		m_layout_pin_all(0),
		m_layout_unpin_all(0),
		m_layout_decorate_panel(0),
		m_layout_undecorate_panel(0),
		m_layout_split_horizontal(0),
		m_layout_split_vertical(0),
		m_layout_kill_panel(0),
		m_document_state(DocumentState)
	{
		base::initialize(DocumentState);

		++m_count;

		document().title().property_changed_signal().connect(sigc::mem_fun(*this, &main_document_window::on_document_title_changed));
		m_fullscreen.changed_signal().connect(sigc::mem_fun(*this, &main_document_window::on_window_fullscreen));
		m_panel_focus_signal.connect(sigc::mem_fun(*this, &main_document_window::on_panel_focus_changed));

		document().state_recorder().connect_current_node_changed_signal(sigc::mem_fun(*this, &main_document_window::update_undo_redo_controls));

		m_document_state.set_cursor_signal().connect(sigc::mem_fun(*this, &main_document_window::on_set_cursor));
		m_document_state.clear_cursor_signal().connect(sigc::mem_fun(*this, &main_document_window::on_clear_cursor));

		Gtk::MenuBar* const menubar = new Gtk::MenuBar();
		menubar->get_accessible()->set_name("main");
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_File"), *manage(create_file_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Edit"), *manage(create_edit_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Select"), *manage(create_select_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Layout"), *manage(create_layout_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_View"), *manage(create_view_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Create"), *manage(create_create_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Modify"), *manage(create_modifier_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Render"), *manage(create_render_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("S_cripting"), *manage(create_scripting_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Advanced"), *manage(create_advanced_menu())));
		menubar->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Help"), *manage(create_help_menu())));
		menubar->show_all();

		m_panel_frame.get_accessible()->set_name("main");
		m_panel_frame.show_all();

		// Setup main box with menubar, panel frame and status bar
		Gtk::VBox* const vbox1 = new Gtk::VBox(false);
		vbox1->get_accessible()->set_name("vbox");
		vbox1->pack_start(*Gtk::manage(menubar), Gtk::PACK_SHRINK);
		vbox1->pack_start(m_panel_frame, Gtk::PACK_EXPAND_WIDGET);
//		vbox1->pack_start(*Gtk::manage(hbox1), Gtk::PACK_SHRINK);
		vbox1->show();

		add(*manage(vbox1));

		// Load layout
		if(!load_ui_layout())
		{
			set_default_layout();
			maximize();
		}

		update_panel_controls(0);

		// Update variables
		on_document_title_changed(0);
		on_window_fullscreen(0);

		show();

		// Give the event-loop a chance to catch-up (otherwise, we get breakage in test cases)
		handle_pending_events();
	}

	void delete_children(Gtk::Widget& Widget)
	{
		if(panel_frame::control* const control = dynamic_cast<panel_frame::control*>(&Widget))
		{
			delete control;
			return;
		}

		if(Gtk::Container* const container = dynamic_cast<Gtk::Container*>(&Widget))
		{
			std::vector<Gtk::Widget*> children = container->get_children();
			for(size_t i = 0; i != children.size(); ++i)
				delete_children(*children[i]);

			delete container;
		}
	}

	~main_document_window()
	{
		if(Gtk::Widget* frame_child = m_panel_frame.get_child())
			delete_children(*frame_child);

		--m_count;
	}

	void save(k3d::xml::element& Element, const k3d::ipersistent::save_context& Context)
	{
		Gtk::Widget* frame_child = m_panel_frame.get_child();
		if(!frame_child)
			return;

		save_ui_container(frame_child, Element);
	}

	void load(k3d::xml::element& Element, const k3d::ipersistent::load_context& Context)
	{
		if(!load_ui_container(Element))
		{
			k3d::log() << warning << "Using default UI layout" << std::endl;
			set_default_layout();
		}
	}

	void set_default_layout()
	{
		if(Gtk::Widget* frame_child = m_panel_frame.get_child())
		{
			m_panel_frame.remove();
			delete frame_child;
		}

		// Create first panel frame
		panel_frame::control* const panel_frame1 = create_panel_frame();
		m_panel_frame.set_shadow_type(Gtk::SHADOW_NONE);
		m_panel_frame.add(*Gtk::manage(panel_frame1));

		// First panel is the toolbar
		panel_frame1->mount_panel("NGUIToolbarPanel");
		panel_frame1->decorations.set_value(false);

		// Node list in the middle
		panel_frame::control* const panel_frame2 = split_panel(*panel_frame1, *Gtk::manage(new Gtk::VPaned), -1);
		panel_frame2->mount_panel("NGUINodeListPanel");

		// Timeline at the bottom
		panel_frame::control* const panel_frame3 = split_panel(*panel_frame2, *Gtk::manage(new Gtk::VPaned), -1, Gtk::SHRINK);
		panel_frame3->mount_panel("NGUITimelinePanel");
		panel_frame3->decorations.set_value(false);

		// Viewport on node list's right
		panel_frame::control* const panel_frame4 = split_panel(*panel_frame2, *Gtk::manage(new Gtk::HPaned), 200);

		const std::vector<k3d::gl::irender_viewport*> gl_engines = k3d::node::lookup<k3d::gl::irender_viewport>(document());
		k3d::gl::irender_viewport* const glengine1 = gl_engines.size() ? gl_engines[0] : 0;

		const std::vector<k3d::icamera*> cameras = k3d::node::lookup<k3d::icamera>(document());
		k3d::icamera* const camera1 = cameras.size() ? cameras[0] : 0;

		if(glengine1 && camera1)
		{
			viewport::control* const control = new viewport::control(m_document_state);
			control->get_accessible()->set_name("viewport");
			control->set_camera(camera1);
			control->set_gl_engine(glengine1);
			panel_frame4->mount_panel(*Gtk::manage(control), "NGUIViewportPanel");
		}

		// Node history below node list
		panel_frame::control* const panel_frame5 = split_panel(*panel_frame2, *Gtk::manage(new Gtk::VPaned), -1);
		panel_frame5->mount_panel("NGUIPipelinePanel");
		// Node properties below node history
		panel_frame::control* const panel_frame6 = split_panel(*panel_frame5, *Gtk::manage(new Gtk::VPaned), -1);
		panel_frame6->mount_panel("NGUINodePropertiesPanel");
		
		set_focus_viewport_panel(panel_frame4);
	}

private:
	void on_set_cursor(const Glib::RefPtr<Gdk::Pixbuf> Cursor)
	{
		if(Cursor)
			get_window()->set_cursor(Gdk::Cursor(get_display(), Cursor, 0, 0));
		else
			get_window()->set_cursor();
	}

	void on_clear_cursor()
	{
		get_window()->set_cursor();
	}

	panel_frame::control* create_panel_frame(/*const std::string& Name*/)
	{
		panel_frame::control* const control = new panel_frame::control(m_document_state, m_panel_focus_signal);
		control->pinned.changed_signal().connect(sigc::mem_fun(*this, &main_document_window::update_panel_controls));
		control->automagic.changed_signal().connect(sigc::mem_fun(*this, &main_document_window::update_panel_controls));
		control->grab_panel_focus();

		return control;
	}

	Gtk::Menu* create_file_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU)),
				_("_New"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_file_new))
			<< set_accelerator_path("<k3d-document>/actions/file/new", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::OPEN, Gtk::ICON_SIZE_MENU)),
				_("_Open..."), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_file_open))
			<< set_accelerator_path("<k3d-document>/actions/file/open", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::OPEN, Gtk::ICON_SIZE_MENU)),
				_("_Merge Nodes..."), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_file_merge_nodes))
			<< set_accelerator_path("<k3d-document>/actions/file/merge_nodes", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::SAVE, Gtk::ICON_SIZE_MENU)),
				_("_Save"), true)
			<< connect_menu_item(sigc::hide_return(sigc::mem_fun(*this, &main_document_window::on_file_save)))
			<< set_accelerator_path("<k3d-document>/actions/file/save", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::SAVE_AS, Gtk::ICON_SIZE_MENU)),
				_("Save _As..."), true)
			<< connect_menu_item(sigc::hide_return(sigc::mem_fun(*this, &main_document_window::on_file_save_as)))
			<< set_accelerator_path("<k3d-document>/actions/file/save_as", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::REVERT_TO_SAVED, Gtk::ICON_SIZE_MENU)),
				_("_Revert"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_file_revert))
			<< set_accelerator_path("<k3d-document>/actions/file/revert", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Import ..."), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_file_import))
			<< set_accelerator_path("<k3d-document>/actions/file/import", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Export ..."), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_file_export))
			<< set_accelerator_path("<k3d-document>/actions/file/export", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU)),
				_("_Close"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_file_close))
			<< set_accelerator_path("<k3d-document>/actions/file/close", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::QUIT, Gtk::ICON_SIZE_MENU)),
				_("_Quit"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_file_quit))
			<< set_accelerator_path("<k3d-document>/actions/file/quit", get_accel_group())));

		return menu;
	}

	Gtk::Menu* create_tools_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Select"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_selection_tool))
			<< set_accelerator_path("<k3d-document>/actions/edit/tools/select_tool", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Move"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_move_tool))
			<< set_accelerator_path("<k3d-document>/actions/edit/tools/move_tool", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Rotate"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_rotate_tool))
			<< set_accelerator_path("<k3d-document>/actions/edit/tools/rotate_tool", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Scale"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_scale_tool))
			<< set_accelerator_path("<k3d-document>/actions/edit/tools/scale_tool", get_accel_group())));

		if(k3d::plugin::factory::lookup("NGUIParentTool"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::MenuItem(_("_Parent"), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_parent_tool))
				<< set_accelerator_path("<k3d-document>/actions/edit/tools/NGUIParentTool", get_accel_group())));
		}

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Unparent"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_unparent))
			<< set_accelerator_path("<k3d-document>/actions/edit/tools/unparent", get_accel_group())));

		if(k3d::plugin::factory::lookup("NGUIRenderRegionTool"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::MenuItem( _("Render R_egion"), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_region_tool))
				<< set_accelerator_path("<k3d-document>/actions/edit/tools/NGUIRenderRegionTool", get_accel_group())));
		}

		if(k3d::plugin::factory::lookup("NGUISnapTool"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::MenuItem(_("S_nap Tool"), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_snap_tool))
				<< set_accelerator_path("<k3d-document>/actions/edit/tools/NGUISnapTool", get_accel_group())));
		}

		return menu;
	}

	Gtk::Menu* create_edit_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		m_undo_menu_item.reset(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::UNDO, Gtk::ICON_SIZE_MENU)),
				_("_Undo"), true)
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_edit_undo), false))
			<< set_accelerator_path("<k3d-document>/actions/edit/undo", get_accel_group()));

		m_undo_all_menu_item.reset(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::UNDO, Gtk::ICON_SIZE_MENU)),
				_("Undo All"))
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_edit_undo), true))
			<< set_accelerator_path("<k3d-document>/actions/edit/undo_all", get_accel_group()));

		m_redo_menu_item.reset(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::REDO, Gtk::ICON_SIZE_MENU)),
				_("_Redo"), true)
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_edit_redo), false))
			<< set_accelerator_path("<k3d-document>/actions/edit/redo", get_accel_group()));

		m_redo_all_menu_item.reset(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::REDO, Gtk::ICON_SIZE_MENU)),
				_("Redo All"))
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_edit_redo), true))
			<< set_accelerator_path("<k3d-document>/actions/edit/redo_all", get_accel_group()));

		menu->items().push_back(*m_undo_menu_item);
		menu->items().push_back(*m_undo_all_menu_item);
		menu->items().push_back(*m_redo_menu_item);
		menu->items().push_back(*m_redo_all_menu_item);
		update_undo_redo_controls();


		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("_Tools"), *Gtk::manage(create_tools_menu())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Instantiate"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_instantiate))
			<< set_accelerator_path("<k3d-document>/actions/edit/instantiate", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("D_uplicate"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_duplicate))
			<< set_accelerator_path("<k3d-document>/actions/edit/duplicate", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Delete"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_delete))
			<< set_accelerator_path("<k3d-document>/actions/edit/delete", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		if(k3d::plugin::factory::lookup("NGUIAssignHotkeysDialog"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::MenuItem(_("Assign _Hotkeys"), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_assign_hotkeys))
				<< set_accelerator_path("<k3d-document>/actions/view/assign_hotkeys", get_accel_group())));
		}

		return menu;
	}

	Gtk::Menu* create_select_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_All"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_all))
			<< set_accelerator_path("<k3d-document>/actions/select/select_all", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_None"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_none))
			<< set_accelerator_path("<k3d-document>/actions/select/select_none", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Invert"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_invert))
			<< set_accelerator_path("<k3d-document>/actions/select/select_invert", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Parent"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_parent))
			<< set_accelerator_path("<k3d-document>/actions/select/select_parent", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Child"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_child))
			<< set_accelerator_path("<k3d-document>/actions/select/select_child", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Sibling"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_sibling))
			<< set_accelerator_path("<k3d-document>/actions/select/select_sibling", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("N_odes"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_nodes))
			<< set_accelerator_path("<k3d-document>/actions/select/select_nodes", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Poin_ts"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_vertices))
			<< set_accelerator_path("<k3d-document>/actions/select/select_points", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Edges"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_split_edges))
			<< set_accelerator_path("<k3d-document>/actions/select/select_split_edges", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Faces"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_faces))
			<< set_accelerator_path("<k3d-document>/actions/select/select_faces", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Curves"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_curves))
			<< set_accelerator_path("<k3d-document>/actions/select/select_curves", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Patches"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_patches))
			<< set_accelerator_path("<k3d-document>/actions/select/select_patches", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Uniform"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_select_uniform))
			<< set_accelerator_path("<k3d-document>/actions/select/select_uniform", get_accel_group())));
		
		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());
		
		return menu;
	}

	Gtk::Menu* create_layout_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		m_layout_maximize_panel = new check_menu_item::control(check_menu_item::proxy(m_maximize_panel), _("_Maximize Panel"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_maximize_panel))
			<< set_accelerator_path("<k3d-document>/actions/layout/maximize_panel", get_accel_group());

		m_layout_hide_unpinned = new check_menu_item::control(check_menu_item::proxy(m_hide_unpinned_panels), _("H_ide Unpinned Panels"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_hide_show_unpinned))
			<< set_accelerator_path("<k3d-document>/actions/layout/hide_unpinned", get_accel_group());

		m_layout_pin_all = new Gtk::MenuItem(_("_Pin All Panels"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_pin_all))
			<< set_accelerator_path("<k3d-document>/actions/layout/pin_all_panels", get_accel_group());

		m_layout_unpin_all = new Gtk::MenuItem(_("_Unpin All Panels"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_unpin_all))
			<< set_accelerator_path("<k3d-document>/actions/layout/unpin_all", get_accel_group());

		m_layout_decorate_panel = new Gtk::MenuItem(_("_Decorate Selected Panel"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_decorate_panel))
			<< set_accelerator_path("<k3d-document>/actions/layout/decorate_panel", get_accel_group());

		m_layout_undecorate_panel = new Gtk::MenuItem(_("U_ndecorate Selected Panel"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_undecorate_panel))
			<< set_accelerator_path("<k3d-document>/actions/layout/undecorate_panel", get_accel_group());

		m_layout_split_horizontal = new Gtk::MenuItem(_("Split Panel _Horizontally"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_split_horizontal))
			<< set_accelerator_path("<k3d-document>/actions/layout/split_horizontal", get_accel_group());

		m_layout_split_vertical = new Gtk::MenuItem(_("Split Panel _Vertically"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_split_vertical))
			<< set_accelerator_path("<k3d-document>/actions/layout/split_vertical", get_accel_group());

		m_layout_kill_panel = new Gtk::MenuItem(_("_Kill Panel"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_kill_panel))
			<< set_accelerator_path("<k3d-document>/actions/layout/kill_panel", get_accel_group());

		menu->items().push_back(*Gtk::manage(m_layout_maximize_panel));

		menu->items().push_back(*Gtk::manage(m_layout_hide_unpinned));
		menu->items().push_back(*Gtk::manage(m_layout_pin_all));
		menu->items().push_back(*Gtk::manage(m_layout_unpin_all));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(m_layout_decorate_panel));
		menu->items().push_back(*Gtk::manage(m_layout_undecorate_panel));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(m_layout_split_horizontal));
		menu->items().push_back(*Gtk::manage(m_layout_split_vertical));
		menu->items().push_back(*Gtk::manage(m_layout_kill_panel));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new check_menu_item::control(check_menu_item::proxy(m_fullscreen), _("_Fullscreen"), true)
			<< set_accelerator_path("<k3d-document>/actions/layout/fullscreen", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Save layout"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_save))
			<< set_accelerator_path("<k3d-document>/actions/layout/save_layout", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Reset layout"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_layout_reset))
			<< set_accelerator_path("<k3d-document>/actions/layout/reset_layout", get_accel_group())));

		return menu;
	}

	Gtk::Menu* create_set_view_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("+_X view"), true)
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_view_set_view), k3d::PX))
			<< set_accelerator_path("<k3d-document>/actions/view/set_view/px_view", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("-X view"))
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_view_set_view), k3d::NX))
			<< set_accelerator_path("<k3d-document>/actions/view/set_view/nx_view", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("+_Y view"), true)
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_view_set_view), k3d::PY))
			<< set_accelerator_path("<k3d-document>/actions/view/set_view/py_view", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("-Y view"))
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_view_set_view), k3d::NY))
			<< set_accelerator_path("<k3d-document>/actions/view/set_view/ny_view", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("+_Z view"), true)
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_view_set_view), k3d::PZ))
			<< set_accelerator_path("<k3d-document>/actions/view/set_view/pz_view", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("-Z view"))
			<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_view_set_view), k3d::NZ))
			<< set_accelerator_path("<k3d-document>/actions/view/set_view/nz_view", get_accel_group())));

		return menu;
	}

	Gtk::Menu* create_view_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Hide Selection"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_view_hide_selection))
			<< set_accelerator_path("<k3d-document>/actions/view/hide_selection", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Show Selection"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_view_show_selection))
			<< set_accelerator_path("<k3d-document>/actions/view/show_selection", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Hide _Unselected"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_view_hide_unselected))
			<< set_accelerator_path("<k3d-document>/actions/view/hide_unselected", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Show _All"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_view_show_all))
			<< set_accelerator_path("<k3d-document>/actions/view/show_all", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Aim Selection"))
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_view_aim_selection))
			<< set_accelerator_path("<k3d-document>/actions/view/aim_selection", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Frame Selection"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_view_frame_selection))
			<< set_accelerator_path("<k3d-document>/actions/view/frame_selection", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Set _Camera ..."), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_view_set_camera))
			<< set_accelerator_path("<k3d-document>/actions/view/set_camera", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Orthographic toggle"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_view_toggle_projection))
			<< set_accelerator_path("<k3d-document>/actions/view/toggle_projection", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Set view"), *Gtk::manage(create_set_view_menu())));

		return menu;
	}

	Gtk::Menu* create_create_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		// Group plugin types by their categories ...
		typedef std::set<k3d::iplugin_factory*, k3d::sort_by_name> sorted_factories_t;
		typedef std::map<std::string, sorted_factories_t> grouped_factories_t;
		grouped_factories_t grouped_factories;

		const k3d::plugin::factory::collection_t factories = k3d::plugin::factory::lookup();
		for(k3d::plugin::factory::collection_t::const_iterator f = factories.begin(); f != factories.end(); ++f)
		{
			// If it isn't a document plugin factory, skip it ...
			if(!dynamic_cast<k3d::idocument_plugin_factory*>(*f))
				continue;

			k3d::iplugin_factory& factory = **f;

			for(k3d::iplugin_factory::categories_t::const_iterator category = factory.categories().begin(); category != factory.categories().end(); ++category)
				grouped_factories[*category].insert(&factory);

			if(factory.categories().empty())
				grouped_factories["<Uncategorized>"].insert(&factory);
		}

		for(grouped_factories_t::const_iterator group = grouped_factories.begin(); group != grouped_factories.end(); ++group)
		{
			Gtk::Menu* const submenu = new Gtk::Menu();
			submenu->set_accel_group(get_accel_group());

			menu->items().push_back(Gtk::Menu_Helpers::MenuElem(group->first, *manage(submenu)));

			const sorted_factories_t& sorted_factories = group->second;
			for(sorted_factories_t::const_iterator f = sorted_factories.begin(); f != sorted_factories.end(); ++f)
			{
				k3d::iplugin_factory& factory = **f;

				submenu->items().push_back(*Gtk::manage(
					create_menu_item(factory)
					<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_create_node), &factory))
					<< set_accelerator_path("<k3d-document>/actions/create/" + factory.name(), get_accel_group())));
			}
		}

		return menu;
	}

	Gtk::Menu* create_modifier_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		// Mesh modifiers
		const factories_t& mesh_modifiers = k3d::ngui::mesh_modifiers();
		if(!mesh_modifiers.empty())
		{
			Gtk::Menu* const submenu = new Gtk::Menu();
			submenu->set_accel_group(get_accel_group());

			menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Mesh"), *manage(submenu)));

			for(factories_t::const_iterator modifier = mesh_modifiers.begin(); modifier != mesh_modifiers.end(); ++modifier)
			{
				k3d::iplugin_factory& factory = **modifier;

				submenu->items().push_back(*Gtk::manage(
					create_menu_item(factory)
					<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_modify_meshes), &factory))
					<< set_accelerator_path("<k3d-document>/actions/modifier/" + factory.name(), get_accel_group())));
			}
		}

		// Transform modifiers
		const factories_t& transform_modifiers = k3d::ngui::transform_modifiers();
		if(!transform_modifiers.empty())
		{
			Gtk::Menu* const submenu = new Gtk::Menu();
			submenu->set_accel_group(get_accel_group());

			menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Transform"), *manage(submenu)));

			for(factories_t::const_iterator modifier = transform_modifiers.begin(); modifier != transform_modifiers.end(); ++modifier)
			{
				k3d::iplugin_factory& factory = **modifier;

				submenu->items().push_back(*Gtk::manage(
					create_menu_item(factory)
					<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_modify_transformations), &factory))
					<< set_accelerator_path("<k3d-document>/actions/modifier/" + factory.name(), get_accel_group())));
			}
		}

		return menu;
	}

	Gtk::Menu* create_render_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		if(k3d::plugin::factory::lookup("NGUIRenderRegionTool"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::ImageMenuItem(
					*Gtk::manage(new Gtk::Image(load_icon("NGUIRenderRegionTool", Gtk::ICON_SIZE_MENU))), _("_Region"), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_region_tool))
				<< set_accelerator_path("<k3d-document>/actions/render/render_region", get_accel_group())));
		}

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(load_icon("render_preview", Gtk::ICON_SIZE_MENU))), _("_Preview"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_preview))
			<< set_accelerator_path("<k3d-document>/actions/render/render_preview", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(load_icon("render_frame", Gtk::ICON_SIZE_MENU))), _("_Frame"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_frame))
			<< set_accelerator_path("<k3d-document>/actions/render/render_frame", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(load_icon("render_animation", Gtk::ICON_SIZE_MENU))), _("_Animation"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_animation))
			<< set_accelerator_path("<k3d-document>/actions/render/render_animation", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(load_icon("render_frame", Gtk::ICON_SIZE_MENU))), _("_Viewport Frame"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_viewport_frame))
			<< set_accelerator_path("<k3d-document>/actions/render/render_viewport_frame", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(load_icon("render_animation", Gtk::ICON_SIZE_MENU))), _("Viewport A_nimation"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_viewport_animation))
			<< set_accelerator_path("<k3d-document>/actions/render/render_viewport_animation", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Set Preview Engine ..."))
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_set_viewport_preview_engine))
			<< set_accelerator_path("<k3d-document>/actions/render/set_preview_engine", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Set Still Engine ..."))
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_set_viewport_still_engine))
			<< set_accelerator_path("<k3d-document>/actions/render/set_still_engine", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("Set Animation Engine ..."))
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_render_set_viewport_animation_engine))
			<< set_accelerator_path("<k3d-document>/actions/render/set_animation_engine", get_accel_group())));

		return menu;
	}

	Gtk::Menu* create_scripting_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Play ..."), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_scripting_play))
			<< set_accelerator_path("<k3d-document>/actions/scripting/play_script", get_accel_group())));

		if(k3d::plugin::factory::lookup("NGUITextEditorDialog"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::MenuItem(_("_Editor ..."), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_scripting_script_editor))
				<< set_accelerator_path("<k3d-document>/actions/scripting/script_editor", get_accel_group())));
		}

		if(k3d::plugin::factory::lookup("NGUIPythonShellDialog"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::MenuItem(_("Python _Shell ..."), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_scripting_python_shell))
				<< set_accelerator_path("<k3d-document>/actions/scripting/python_shell", get_accel_group())));
		}

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		// Create menu items for (optional) scripted actions ...
		Gtk::Menu* actions_menu = 0;

		const k3d::plugin::factory::collection_t factories = k3d::plugin::factory::lookup();
		for(k3d::plugin::factory::collection_t::const_iterator factory = factories.begin(); factory != factories.end(); ++factory)
		{
			k3d::iplugin_factory::metadata_t metadata = (**factory).metadata();

			if(!metadata.count("ngui:action"))
				continue;

			if(!actions_menu)
			{
				actions_menu = new Gtk::Menu();
				actions_menu->set_accel_group(get_accel_group());
				menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Actions"), *manage(actions_menu)));
			}

			actions_menu->items().push_back(*Gtk::manage(
				create_menu_item(**factory)
				<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_scripting_action), *factory))
				<< set_accelerator_path("<k3d-document>/actions/scripting/action/" + (**factory).name(), get_accel_group())));
		}

		return menu;
	}

	Gtk::Menu* create_advanced_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		const k3d::plugin::factory::collection_t dialogs = k3d::plugin::factory::lookup("ngui:component-type", "dialog");
		if(!dialogs.empty())
		{
			std::vector<k3d::iplugin_factory*> sorted_dialogs(dialogs.begin(), dialogs.end());
			std::sort(sorted_dialogs.begin(), sorted_dialogs.end(), k3d::sort_by_name());

			Gtk::Menu* const dialogs_menu = new Gtk::Menu();
			menu->items().push_back(Gtk::Menu_Helpers::MenuElem(_("Dialogs"), *manage(dialogs_menu)));

			for(std::vector<k3d::iplugin_factory*>::iterator dialog = sorted_dialogs.begin(); dialog != sorted_dialogs.end(); ++dialog)
			{
				dialogs_menu->items().push_back(*Gtk::manage(
					create_menu_item(**dialog)
					<< connect_menu_item(sigc::bind(sigc::mem_fun(*this, &main_document_window::on_advanced_create_dialog), *dialog))
					<< set_accelerator_path("<k3d-document>/actions/advanced/create_dialog/" + (**dialog).name(), get_accel_group())));
			}
		}

		return menu;
	}

	Gtk::Menu* create_help_menu()
	{
		Gtk::Menu* const menu = new Gtk::Menu();
		menu->set_accel_group(get_accel_group());

		if(k3d::plugin::factory::lookup("NGUILearningDialog"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::MenuItem(_("Example documents ..."), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_help_learning_menu))
				<< set_accelerator_path("<k3d-document>/actions/help/learning_menu", get_accel_group())));
		}

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("File _Bug Report ..."), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_help_file_bug_report))
			<< set_accelerator_path("<k3d-document>/actions/help/file_bug_report", get_accel_group())));

		if(k3d::plugin::factory::lookup("NGUILogDialog"))
		{
			menu->items().push_back(*Gtk::manage(
			    new Gtk::MenuItem(_("Open _Log Window ..."), true)
			    << connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_help_open_log_window))
			    << set_accelerator_path("<k3d-document>/actions/help/open_log_window", get_accel_group())));
		}

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		menu->items().push_back(*Gtk::manage(
			new Gtk::ImageMenuItem(
				*Gtk::manage(new Gtk::Image(Gtk::Stock::HELP, Gtk::ICON_SIZE_MENU)),
				_("_K-3D Guide"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_help_guide))
			<< set_accelerator_path("<k3d-document>/actions/help/guide", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("_Release Notes"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_help_release_notes))
			<< set_accelerator_path("<k3d-document>/actions/help/release_notes", get_accel_group())));

		menu->items().push_back(*Gtk::manage(
			new Gtk::MenuItem(_("K-3D _Online"), true)
			<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_help_online))
			<< set_accelerator_path("<k3d-document>/actions/help/online", get_accel_group())));

		menu->items().push_back(Gtk::Menu_Helpers::SeparatorElem());

		if(k3d::plugin::factory::lookup("NGUIAboutDialog"))
		{
			menu->items().push_back(*Gtk::manage(
				new Gtk::MenuItem(_("_About K-3D ..."), true)
				<< connect_menu_item(sigc::mem_fun(*this, &main_document_window::on_help_about))
				<< set_accelerator_path("<k3d-document>/actions/help/about", get_accel_group())));
		}

		return menu;
	}

	bool on_delete_event(GdkEventAny*)
	{
		on_file_close();
		return true;
	}

	void on_assign_hotkeys()
	{
		// Note: the plugin will only open one dialog at-a-time, so it isn't an error if we don't get one back ...
		if(Gtk::Window* const window = k3d::plugin::create<Gtk::Window>("NGUIAssignHotkeysDialog"))
			window->set_transient_for(*this);
	}

	void on_file_new()
	{
		create_document();
	}

	void on_file_open()
	{
		k3d::filesystem::path document_path;
		{
			file_chooser_dialog dialog(_("Open K-3D Document:"), k3d::options::path::documents(), Gtk::FILE_CHOOSER_ACTION_OPEN);
			dialog.add_pattern_filter(_("K-3D Document (*.k3d)"), "*.k3d");
			dialog.add_all_files_filter();
			if(!dialog.get_file_path(document_path))
				return;
		}

		open_document(document_path);
	}

	void on_file_merge_nodes()
	{
		merge_nodes::merge_nodes(document());
	}

	bool on_file_save()
	{
		const k3d::filesystem::path document_path = boost::any_cast<k3d::filesystem::path>(document().path().property_internal_value());

		if(document_path.empty())
			return on_file_save_as();

		boost::scoped_ptr<k3d::idocument_exporter> exporter(k3d::plugin::create<k3d::idocument_exporter>(k3d::classes::DocumentExporter()));
		return_val_if_fail(exporter, false);

		if(!exporter->write_file(document(), document_path))
		{
			error_message(_("File could not be saved"));
			return false;
		}

		return true;
	}

	bool on_file_save_as()
	{
		k3d::filesystem::path document_path;

		{
			file_chooser_dialog dialog(_("Save K-3D Document As:"), k3d::options::path::documents(), Gtk::FILE_CHOOSER_ACTION_SAVE);
			dialog.add_pattern_filter(_("K-3D Document (*.k3d)"), "*.k3d");
			dialog.add_all_files_filter();
			dialog.append_extension(".k3d");

			if(!dialog.get_file_path(document_path))
				return false;
		}

		boost::scoped_ptr<k3d::idocument_exporter> exporter(k3d::plugin::create<k3d::idocument_exporter>(k3d::classes::DocumentExporter()));
		return_val_if_fail(exporter.get(), false);

		if(!exporter->write_file(document(), document_path))
		{
			error_message(_("File could not be saved"));
			return false;
		}

		// Record the new path & title ...
		//k3d::property::set_internal_value(document().path(), document_path);
		//k3d::property::set_internal_value(document().title(), document_path.leaf());

		return true;
	}

	class import_export_columns :
		public Gtk::TreeModelColumnRecord
	{
	public:
		import_export_columns()
		{
			add(factory);
			add(label);
			add(icon);
		}

		Gtk::TreeModelColumn<k3d::iplugin_factory*> factory;
		Gtk::TreeModelColumn<Glib::ustring> label;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
	};

	void file_revert()
	{
		boost::scoped_ptr<k3d::idocument_importer> importer(k3d::plugin::create<k3d::idocument_importer>(k3d::classes::DocumentImporter()));
		if(!importer)
		{
			error_message(_("Document reader plugin not installed."));
			return;
		}

		const k3d::filesystem::path document_path = boost::any_cast<k3d::filesystem::path>(document().path().property_internal_value());

		k3d::idocument* const reverted_document = k3d::application().create_document();
		return_if_fail(reverted_document);

		if(!importer->read_file(document_path, *reverted_document))
		{
			error_message(_("Error reading document.  The document could not be reverted."));
			return;
		}

		document_state* const state = new document_state(*reverted_document);
		create_main_document_window(*state);

		k3d::property::set_internal_value(reverted_document->path(), document_path);
		k3d::property::set_internal_value(reverted_document->title(), document_path.leaf());

		k3d::application().close_document(document());
	}

	void on_file_revert()
	{
		const k3d::ustring document_title = boost::any_cast<k3d::ustring>(document().title().property_internal_value());

		std::vector<std::string> buttons;
		buttons.push_back(_("Revert"));
		buttons.push_back(_("Cancel"));
		std::string message = k3d::string_cast(boost::format(_("Revert %1% to last-saved version? Unsaved changes will be lost (No Undo)")) % document_title.raw());

		const int choice = query_message(message, 1, buttons);
		switch(choice)
		{
			case 0:
				return;
			case 1:
				file_revert();
				return;
			case 2:
				return;
		}
	}

	void on_file_import()
	{
		// Make sure we have some file formats to choose from ...
		const k3d::plugin::factory::collection_t factories = k3d::plugin::factory::lookup<k3d::idocument_importer>();
		if(factories.empty())
		{
			error_message(_("No import file filters available"));
			return;
		}

		import_export_columns columns;
		Glib::RefPtr<Gtk::ListStore> model = Gtk::ListStore::create(columns);

		Gtk::TreeRow row = *model->append();
		row[columns.factory] = 0;
		row[columns.label] = _("Automatic file detection");

		for(k3d::plugin::factory::collection_t::const_iterator factory = factories.begin(); factory != factories.end(); ++factory)
		{
			Gtk::TreeRow row = *model->append();
			row[columns.factory] = *factory;
			row[columns.icon] = quiet_load_icon((*factory)->name(), Gtk::ICON_SIZE_MENU);
			row[columns.label] = (*factory)->short_description();
		}

		// Prompt the user for a file to import ...
		k3d::filesystem::path filepath;
		boost::scoped_ptr<k3d::idocument_importer> importer;

		{
			file_chooser_dialog dialog(_("Import Document:"), k3d::options::path::documents(), Gtk::FILE_CHOOSER_ACTION_OPEN);

			Gtk::HBox import_box(false, 5);
			Gtk::Label import_label(_("Choose import plugin:"));

			Gtk::ComboBox import_combo(Glib::RefPtr<Gtk::TreeModel>::cast_dynamic(model));
			import_combo.pack_start(columns.icon, false);
			import_combo.pack_start(columns.label);
			import_combo.set_active(0);

			import_box.pack_start(import_label, Gtk::PACK_SHRINK);
			import_box.pack_start(import_combo, Gtk::PACK_EXPAND_WIDGET);
			import_box.show_all();

			dialog.add_extra_widget(import_box);

			if(!dialog.get_file_path(filepath))
				return;

			return_if_fail(import_combo.get_active() != model->children().end());

			if(k3d::iplugin_factory* factory = import_combo.get_active()->get_value(columns.factory))
			{
				importer.reset(k3d::plugin::create<k3d::idocument_importer>(*factory));
			}
			else
			{
				const k3d::mime::type mime_type = k3d::mime::type::lookup(filepath);
				if(mime_type.empty())
				{
					error_message(
						_("Could not detect filetype automatically.\n"
						"Try choosing a specific filter that matches the file to be imported"));
					return;
				}

				const k3d::plugin::factory::collection_t factories = k3d::plugin::factory::lookup<k3d::idocument_importer>(mime_type);
				if(factories.size() != 1)
				{
					error_message(k3d::string_cast(boost::format(_("No plugin installed that can import documents of type %1%.")) % mime_type));
					return;
				}

				importer.reset(k3d::plugin::create<k3d::idocument_importer>(**factories.begin()));
			}

			if(!importer)
			{
				error_message(_("Error creating a plugin to load the document."));
			}
		}

		// Make this an undoable operation ...
		k3d::record_state_change_set change_set(document(), k3d::string_cast(boost::format(_("Import %1%")) % filepath.native_utf8_string().raw()), K3D_CHANGE_SET_CONTEXT);
		if(!importer->read_file(filepath, document()))
		{
			error_message(
				"Error importing file.  If you chose \"Automatic\" as the filter type,\n"
				"try choosing a specific filter that matches the filetype you're importing.");
			return;
		}

		k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_file_export()
	{
		// Make sure we have some file formats to choose from ...
		const k3d::plugin::factory::collection_t factories = k3d::plugin::factory::lookup<k3d::idocument_exporter>();
		if(factories.empty())
		{
			error_message(_("No export file filters available"));
			return;
		}

		import_export_columns columns;
		Glib::RefPtr<Gtk::ListStore> model = Gtk::ListStore::create(columns);
		for(k3d::plugin::factory::collection_t::const_iterator factory = factories.begin(); factory != factories.end(); ++factory)
		{
			Gtk::TreeRow row = *model->append();
			row[columns.factory] = *factory;
			row[columns.icon] = quiet_load_icon((*factory)->name(), Gtk::ICON_SIZE_MENU);
			row[columns.label] = (*factory)->short_description();
		}

		// Prompt the user for a file to export ...
		k3d::filesystem::path filepath;
		boost::scoped_ptr<k3d::idocument_exporter> exporter;

		{
			file_chooser_dialog dialog(_("Export Document:"), k3d::options::path::documents(), Gtk::FILE_CHOOSER_ACTION_SAVE);

			Gtk::HBox export_box(false, 5);
			Gtk::Label export_label(_("Choose export plugin:"));

			Gtk::ComboBox export_combo(Glib::RefPtr<Gtk::TreeModel>::cast_dynamic(model));
			export_combo.pack_start(columns.icon, false);
			export_combo.pack_start(columns.label);
			export_combo.set_active(0);

			export_box.pack_start(export_label, Gtk::PACK_SHRINK);
			export_box.pack_start(export_combo, Gtk::PACK_EXPAND_WIDGET);
			export_box.show_all();

			dialog.add_extra_widget(export_box);

			if(!dialog.get_file_path(filepath))
				return;

			return_if_fail(export_combo.get_active() != model->children().end());
			if(k3d::iplugin_factory* factory = export_combo.get_active()->get_value(columns.factory))
			{
				exporter.reset(k3d::plugin::create<k3d::idocument_exporter>(*factory));
			}

			return_if_fail(exporter.get());
		}

		if(!exporter->write_file(document(), filepath))
			error_message(_("Error exporting document"));
	}

	void on_file_close()
	{
		if(m_count == 1)
		{
			application_state::instance().safe_close(*this);
		}
		else
		{
			m_document_state.safe_close(*this);
		}
	}

	void on_file_quit()
	{
		application_state::instance().safe_close(*this);
	}

	void on_edit_undo(const bool All)
	{
		const k3d::istate_recorder::node* const current_node = document().state_recorder().current_node();
		return_if_fail(current_node);

		current_node->change_set->undo();
		document().state_recorder().set_current_node(current_node->parent);

		const std::string top_node_name = current_node->label;
		while(All)
		{
			const k3d::istate_recorder::node* const previous_node = document().state_recorder().current_node();
			if(!previous_node)
				break;

			if(top_node_name != previous_node->label)
				break;

			previous_node->change_set->undo();
			document().state_recorder().set_current_node(previous_node->parent);
		}

		k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_edit_redo(const bool All)
	{
		const k3d::istate_recorder::node* const redo_node = next_redo(document().state_recorder());
		return_if_fail(redo_node);

		redo_node->change_set->redo();
		document().state_recorder().set_current_node(redo_node);

		const std::string initial_node_name = redo_node->label;
		while(All)
		{
			const k3d::istate_recorder::node* const next_node = next_redo(document().state_recorder());
			if(!next_node)
				break;

			if(initial_node_name != next_node->label)
				break;

			next_node->change_set->redo();
			document().state_recorder().set_current_node(next_node);
		}

		k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_selection_tool()
	{
		m_document_state.set_active_tool(m_document_state.selection_tool());
	}

	void on_move_tool()
	{
		m_document_state.set_active_tool(m_document_state.move_tool());
	}

	void on_rotate_tool()
	{
		m_document_state.set_active_tool(m_document_state.rotate_tool());
	}

	void on_scale_tool()
	{
		m_document_state.set_active_tool(m_document_state.scale_tool());
	}

	void on_parent_tool()
	{
		tool* const parent_tool = m_document_state.get_tool("NGUIParentTool");
		return_if_fail(parent_tool);

		m_document_state.set_active_tool(*parent_tool);
	}

	void on_snap_tool()
	{
		tool* const snap_tool = m_document_state.get_tool("NGUISnapTool");
		return_if_fail(snap_tool);

		m_document_state.set_active_tool(*snap_tool);
	}

	void on_instantiate()
	{
		pipeline::instantiate_selected_nodes(m_document_state.document());
	}

	void on_duplicate()
	{
		pipeline::duplicate_selected_nodes(m_document_state.document());
	}

	void on_delete()
	{
		k3d::record_state_change_set changeset(m_document_state.document(), _("Delete nodes"), K3D_CHANGE_SET_CONTEXT);

		k3d::nodes_t nodes = selection::state(m_document_state.document()).selected_nodes();
		k3d::delete_nodes(m_document_state.document(), nodes);

		k3d::gl::redraw_all(m_document_state.document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_render_region_tool()
	{
		tool* const render_region_tool = m_document_state.get_tool("NGUIRenderRegionTool");
		return_if_fail(render_region_tool);

		m_document_state.set_active_tool(*render_region_tool);
	}

	void on_select_all()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select All"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).select_all();
	}

	void on_select_none()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select None"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).deselect_all();
	}

	void on_select_invert()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select Invert"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).invert_selection();
	}

	void on_select_parent()
	{
		// Keep track of the set of parent nodes to be selected
		std::set<k3d::inode*> parents;

		const k3d::inode_collection::nodes_t& nodes = m_document_state.document().nodes().collection();
		for(k3d::inode_collection::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
		{
			// Skip unselected nodes
			if(!selection::state(m_document_state.document()).is_selected(**node))
				continue;

			// Skip non-parentable nodes
			if(k3d::iparentable* const parentable = dynamic_cast<k3d::iparentable*>(*node))
				parents.insert(boost::any_cast<k3d::inode*>(parentable->parent().property_internal_value()));
		}

		// Don't let NULL creep in ...
		parents.erase(static_cast<k3d::inode*>(0));

		if(parents.empty())
			return;

		k3d::record_state_change_set change_set(m_document_state.document(), _("Select parent"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).deselect_all();
		std::for_each(parents.begin(), parents.end(), detail::select(m_document_state.document()));
	}

	void on_select_child()
	{
		std::set<k3d::inode*> children;

		const k3d::inode_collection::nodes_t& nodes = m_document_state.document().nodes().collection();
		for(k3d::inode_collection::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
		{
			// Skip non-parentable nodes
			k3d::iparentable* const parentable = dynamic_cast<k3d::iparentable*>(*node);
			if(!parentable)
				continue;

			// Get parent
			k3d::inode* const parent = boost::any_cast<k3d::inode*>(parentable->parent().property_internal_value());
			if(!parent)
				continue;

			if(selection::state(m_document_state.document()).is_selected(*parent))
				children.insert(*node);
		}

		if(children.empty())
			return;

		k3d::record_state_change_set change_set(m_document_state.document(), _("Select child"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).deselect_all();
		std::for_each(children.begin(), children.end(), detail::select(m_document_state.document()));
	}

	void on_select_sibling()
	{
		std::set<k3d::inode*> parents;

		const k3d::inode_collection::nodes_t& nodes = m_document_state.document().nodes().collection();
		for(k3d::inode_collection::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
		{
			// Skip unselected nodes
			if(!selection::state(m_document_state.document()).is_selected(**node))
				continue;

			// Skip non-parentable nodes
			k3d::iparentable* const parentable = dynamic_cast<k3d::iparentable*>(*node);
			if(!parentable)
				continue;

			// Get parent
			k3d::inode* const parent = boost::any_cast<k3d::inode*>(parentable->parent().property_internal_value());
			if(!parent)
				continue;

			parents.insert(parent);
		}

		std::set<k3d::inode*> siblings;

		for(k3d::inode_collection::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
		{
			// Skip selected nodes
			if(selection::state(m_document_state.document()).is_selected(**node))
				continue;

			// Skip non-parentable nodes
			k3d::iparentable* const parentable = dynamic_cast<k3d::iparentable*>(*node);
			if(!parentable)
				continue;

			// Get parent
			k3d::inode* const parent = boost::any_cast<k3d::inode*>(parentable->parent().property_internal_value());
			if(!parent)
				continue;

			if(parents.count(parent))
				siblings.insert(*node);
		}

		if(siblings.empty())
			return;

		k3d::record_state_change_set change_set(m_document_state.document(), _("Select sibling"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).deselect_all();
		std::for_each(siblings.begin(), siblings.end(), detail::select(m_document_state.document()));
	}

	void on_select_curves()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select Curves"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).set_current_mode(selection::CURVE);
	}

	void on_select_patches()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select Patches"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).set_current_mode(selection::PATCH);
	}

	void on_select_faces()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select Polygon Faces"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).set_current_mode(selection::FACE);
	}

	void on_select_nodes()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select Nodes"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).set_current_mode(selection::NODE);
	}

	void on_select_vertices()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select Points"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).set_current_mode(selection::POINT);
	}

	void on_select_split_edges()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select Polygon Edges"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).set_current_mode(selection::EDGE);
	}

	void on_select_uniform()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Select Surface"), K3D_CHANGE_SET_CONTEXT);
		selection::state(m_document_state.document()).set_current_mode(selection::SURFACE);
	}

	void on_layout_maximize_panel()
	{
		return_if_fail(m_focus_panel);

		if(m_maximize_panel.internal_value())
		{
			const panel_frame::controls panel_frames = detail::get_panel_frames(m_panel_frame);
			for(panel_frame::controls::const_iterator panel_frame = panel_frames.begin(); panel_frame != panel_frames.end(); ++panel_frame)
				if(*panel_frame != m_focus_panel)
					(*panel_frame)->hide();

			// Hide panes containing hidden panels (or panes) only
			return_if_fail(m_panel_frame.get_children().size() == 1);

			Gtk::Widget* frame_child = *m_panel_frame.get_children().begin();
			hide_panes(frame_child);

			// Update menu entry
			//->set_label("Show All Panels");
		}
		else
		{
			show_all_panels();

			if(m_hide_unpinned_panels.internal_value())
				hide_unpinned_panels();

			// Update menu entry
			//->set_label("Hide Unpinned Panels");
		}

		update_panel_controls(0);
	}

	void on_layout_hide_show_unpinned()
	{
		if(m_hide_unpinned_panels.internal_value())
		{
			hide_unpinned_panels();

			// Update menu entry
			//->set_label("Show All Panels");
		}
		else
		{
			show_all_panels();

			// Update menu entry
			//->set_label("Hide Unpinned Panels");
		}

		update_panel_controls(0);
	}

	void hide_unpinned_panels()
	{
		const panel_frame::controls panel_frames = detail::get_panel_frames(m_panel_frame);
		for(panel_frame::controls::const_iterator panel_frame = panel_frames.begin(); panel_frame != panel_frames.end(); ++panel_frame)
			if(!(*panel_frame)->pinned.internal_value())
				(*panel_frame)->hide();

		// Hide panes containing hidden panels (or panes) only
		return_if_fail(m_panel_frame.get_children().size() == 1);

		Gtk::Widget* frame_child = *m_panel_frame.get_children().begin();
		hide_panes(frame_child);
	}

	/// Recursively hides panes containing only hidden panels (or panes)
	bool hide_panes(Gtk::Widget* Widget)
	{
		Gtk::Paned* gtk_paned = dynamic_cast<Gtk::Paned*>(Widget);
		if(gtk_paned)
		{
			if(!hide_panes(gtk_paned->get_child1()) && !hide_panes(gtk_paned->get_child2()))
			{
				// Pane contains only hidden children: hide it
				gtk_paned->hide();
				return false;
			}

			return true;
		}

		panel_frame::control* k3d_panel = dynamic_cast<panel_frame::control*>(Widget);
		if(k3d_panel)
			return k3d_panel->is_visible();

		assert_not_reached();

		return false;
	}

	void show_all_panels()
	{
		return_if_fail(m_panel_frame.get_children().size() == 1);

		Gtk::Widget* frame_child = *m_panel_frame.get_children().begin();
		show_all_panels(frame_child);
	}

	/// Recursively shows all panels and panes
	void show_all_panels(Gtk::Widget* Widget)
	{
		return_if_fail(Widget);

		Gtk::Paned* gtk_paned = dynamic_cast<Gtk::Paned*>(Widget);
		if(gtk_paned)
		{
			show_all_panels(gtk_paned->get_child1());
			show_all_panels(gtk_paned->get_child2());
		}

		Widget->show();
	}

	void on_layout_pin_all()
	{
		const panel_frame::controls panel_frames = detail::get_panel_frames(m_panel_frame);
		for(panel_frame::controls::const_iterator panel_frame = panel_frames.begin(); panel_frame != panel_frames.end(); ++panel_frame)
			(*panel_frame)->pinned.set_value(true);
	}

	void on_layout_unpin_all()
	{
		const panel_frame::controls panel_frames = detail::get_panel_frames(m_panel_frame);
		for(panel_frame::controls::const_iterator panel_frame = panel_frames.begin(); panel_frame != panel_frames.end(); ++panel_frame)
			(*panel_frame)->pinned.set_value(false);
	}

	void on_layout_decorate_panel()
	{
		if(!m_focus_panel)
			return;

		m_focus_panel->decorations.set_value(true);

		m_layout_decorate_panel->set_sensitive(false);
		m_layout_undecorate_panel->set_sensitive(true);
	}

	void on_layout_undecorate_panel()
	{
		if(!m_focus_panel)
			return;

		m_focus_panel->decorations.set_value(false);

		m_layout_decorate_panel->set_sensitive(true);
		m_layout_undecorate_panel->set_sensitive(false);
	}

	/// Moves Panel inside NewPaned, creates, inserts and returns new panel_frame::control
	panel_frame::control* split_panel(panel_frame::control& Panel, Gtk::Paned& NewPaned, const int NewPosition, Gtk::AttachOptions AttachOptions = Gtk::EXPAND)
	{
		if(Panel.get_parent() == &m_panel_frame)
		{
			NewPaned.get_accessible()->set_name(Panel.get_parent()->get_accessible()->get_name());
			// Move Panel inside NewPaned
			Panel.reparent(NewPaned);
			// Replace Panel with NewPaned
			m_panel_frame.add(NewPaned);
		}
		else
		{
			// Make sure parent is a Gtk::Paned widget
			Gtk::Paned* const parent_paned = dynamic_cast<Gtk::Paned*>(Panel.get_parent());
			return_val_if_fail(parent_paned, 0);

			const k3d::bool_t horizontal = dynamic_cast<Gtk::HPaned*>(parent_paned) ? true : false;

			// Move Panel inside NewPaned, and put NewPaned instead
			if(parent_paned->get_child1() == &Panel)
			{
				// Panel is parent_paned's child 1
				Panel.reparent(NewPaned);
				parent_paned->pack1(NewPaned, Gtk::EXPAND);
				if(horizontal)
					NewPaned.get_accessible()->set_name("left");
				else
					NewPaned.get_accessible()->set_name("top");
			}
			else
			{
				// Panel is parent_paned's child 2
				Panel.reparent(NewPaned);
				parent_paned->pack2(NewPaned, Gtk::EXPAND);
				if(horizontal)
					NewPaned.get_accessible()->set_name("right");
				else
					NewPaned.get_accessible()->set_name("bottom");
			}
		}

		// Create new panel and insert it as child 2 (Panel is child 1)
		panel_frame::control* const new_panel_frame = create_panel_frame();

		NewPaned.pack2(*manage(new_panel_frame), AttachOptions);
		NewPaned.set_position(NewPosition);

		NewPaned.show();

		update_panel_controls(0);

		return new_panel_frame;
	}

	void on_layout_split_horizontal()
	{
		return_if_fail(m_focus_panel && m_focus_panel->mounted_panel());

		const std::string panel_type = m_focus_panel->mounted_panel()->panel_type();
		panel_frame::control* new_panel = split_panel(*m_focus_panel, *manage(new Gtk::VPaned), m_focus_panel->get_height() / 2);

		new_panel->mount_panel(panel_type, true);
		set_focus_viewport_panel(new_panel);
	}

	void on_layout_split_vertical()
	{
		return_if_fail(m_focus_panel && m_focus_panel->mounted_panel());

		const std::string panel_type = m_focus_panel->mounted_panel()->panel_type();
		panel_frame::control* new_panel = split_panel(*m_focus_panel, *manage(new Gtk::HPaned), m_focus_panel->get_width() / 2);

		new_panel->mount_panel(panel_type, true);
		set_focus_viewport_panel(new_panel);
	}

	void on_layout_kill_panel()
	{
		return_if_fail(m_focus_panel);

		if(m_focus_viewport_panel == m_focus_panel)
			unset_focus_viewport_panel();

		Gtk::Paned* const old_paned = dynamic_cast<Gtk::Paned*>(m_focus_panel->get_parent());
		return_if_fail(old_paned);

		Gtk::Container* const old_paned_parent = dynamic_cast<Gtk::Container*>(old_paned->get_parent());
		return_if_fail(old_paned_parent);

		Gtk::Frame temp_storage;
		old_paned->reparent(temp_storage);

		if(old_paned->get_child1() == m_focus_panel)
		{
			old_paned->get_child2()->reparent(*old_paned_parent);
		}
		else
		{
			old_paned->get_child1()->reparent(*old_paned_parent);
		}

		delete m_focus_panel;
		delete old_paned;

		update_panel_controls(0);
	}

	void on_layout_save()
	{
		save_ui_layout();
		message(_("Window layout saved.  Your custom layout will be used whenever you open a document."));
	}

	void on_layout_reset()
	{
		k3d::filesystem::remove(detail::ui_layout_path());
		message(_("Window layout reset.  The default layout will be used the next time you open a document."));
	}

	void on_view_hide_selection()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Hide selection"), K3D_CHANGE_SET_CONTEXT);

		const k3d::nodes_t selected_nodes = selection::state(m_document_state.document()).selected_nodes();
		for(k3d::nodes_t::const_iterator node = selected_nodes.begin(); node != selected_nodes.end(); ++node)
			node::hide(**node);

		k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_view_show_selection()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Show selection"), K3D_CHANGE_SET_CONTEXT);

		const k3d::nodes_t selected_nodes = selection::state(m_document_state.document()).selected_nodes();
		for(k3d::nodes_t::const_iterator node = selected_nodes.begin(); node != selected_nodes.end(); ++node)
			node::show(**node);

		k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_view_hide_unselected()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Hide unselected"), K3D_CHANGE_SET_CONTEXT);

		const k3d::nodes_t& nodes = m_document_state.document().nodes().collection();
		for(k3d::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
		{
			if(!selection::state(m_document_state.document()).is_selected(**node))
				node::hide(**node);
		}

		k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_view_show_all()
	{
		k3d::record_state_change_set change_set(m_document_state.document(), _("Show all"), K3D_CHANGE_SET_CONTEXT);

		const k3d::nodes_t& nodes = m_document_state.document().nodes().collection();
		for(k3d::nodes_t::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
			node::show(**node);

		k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_view_aim_selection()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::record_state_change_set change_set(m_document_state.document(), _("Aim selection"), K3D_CHANGE_SET_CONTEXT);
		aim_selection(m_document_state, *viewport_control);
	}

	void on_view_frame_selection()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::record_state_change_set change_set(m_document_state.document(), _("Frame selection"), K3D_CHANGE_SET_CONTEXT);
		frame_selection(m_document_state, *viewport_control);
	}

	void on_view_set_camera()
	{
		viewport::control* const viewport = m_document_state.get_focus_viewport();
		return_if_fail(viewport);

		k3d::icamera* const camera = pick_camera(m_document_state, viewport->camera());
		if(!camera)
			return;

		viewport->set_camera(camera);
	}

	void on_view_toggle_projection()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::icamera* camera = viewport_control->camera();
		assert_warning(k3d::property::set_internal_value(*camera, "orthographic", !boost::any_cast<bool>(k3d::property::pipeline_value(*camera, "orthographic"))));
	}

	void on_view_set_view(const k3d::signed_axis Axis)
	{
		// Get camera/target distance
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);
		k3d::icamera* camera = viewport_control->camera();

		const k3d::matrix4 transform_matrix = boost::any_cast<k3d::matrix4>(k3d::property::pipeline_value(camera->transformation().matrix_source_output()));
		const k3d::point3 world_position = transform_matrix * k3d::point3(0, 0, 0);
		const k3d::point3 world_target = boost::any_cast<k3d::point3>(camera->world_target().property_internal_value());
		const double distance = k3d::distance(world_position, world_target);

		k3d::point3 position;
		k3d::vector3 look_vector;
		k3d::vector3 up_vector;
		k3d::vector3 right_vector;
		switch(Axis)
		{
			case k3d::PX:
				position = k3d::point3(distance, 0, 0);
				look_vector = k3d::vector3(-1, 0, 0);
				up_vector = k3d::vector3(0, 0, 1);
				right_vector = k3d::vector3(0, -1, 0);
			break;
			case k3d::NX:
				position = k3d::point3(-distance, 0, 0);
				look_vector = k3d::vector3(1, 0, 0);
				up_vector = k3d::vector3(0, 0, 1);
				right_vector = k3d::vector3(0, 1, 0);
			break;
			case k3d::PY:
				position = k3d::point3(0, distance, 0);
				look_vector = k3d::vector3(0, -1, 0);
				up_vector = k3d::vector3(0, 0, 1);
				right_vector = k3d::vector3(1, 0, 0);
			break;
			case k3d::NY:
				position = k3d::point3(0, -distance, 0);
				look_vector = k3d::vector3(0, 1, 0);
				up_vector = k3d::vector3(0, 0, 1);
				right_vector = k3d::vector3(-1, 0, 0);
			break;
			case k3d::PZ:
				position = k3d::point3(0, 0, distance);
				look_vector = k3d::vector3(0, 0, -1);
				up_vector = k3d::vector3(0, -1, 0);
				right_vector = k3d::vector3(1, 0, 0);
			break;
			case k3d::NZ:
				position = k3d::point3(0, 0, -distance);
				look_vector = k3d::vector3(0, 0, 1);
				up_vector = k3d::vector3(0, 1, 0);
				right_vector = k3d::vector3(1, 0, 0);
			break;
		}

		const k3d::matrix4 view_matrix = k3d::view_matrix(look_vector, up_vector, world_target + position);
		viewport_control->set_view_matrix(view_matrix);
	}

	void on_create_node(k3d::iplugin_factory* const Factory)
	{
		if(cancel_plugin(*Factory))
			return;

		pipeline::create_node(m_document_state.document(), *Factory);
	}

	void on_modify_meshes(k3d::iplugin_factory* Modifier)
	{
		if(cancel_plugin(*Modifier))
			return;

		modify_selected_meshes(m_document_state, Modifier);
		k3d::gl::redraw_all(m_document_state.document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_modify_transformations(k3d::iplugin_factory* Modifier)
	{
		if(cancel_plugin(*Modifier))
			return;

		k3d::nodes_t selected_nodes = selection::state(m_document_state.document()).selected_nodes();

		k3d::inode* new_modifier;
		for(k3d::nodes_t::iterator selected_node = selected_nodes.begin(); selected_node != selected_nodes.end(); ++selected_node)
		{
			new_modifier = modify_transformation(m_document_state.document(), **selected_node, Modifier);
			assert_warning(new_modifier);
		}

		// Show the new modifier properties if only one was processed
		if(selected_nodes.size() == 1)
			panel::mediator(m_document_state.document()).set_focus(*new_modifier);

		k3d::gl::redraw_all(m_document_state.document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}

	void on_scripting_play()
	{
		k3d::filesystem::path filepath;

		// Close file chooser dialog before executing script
		{
			file_chooser_dialog dialog(_("Play Script:"), k3d::options::path::scripts(), Gtk::FILE_CHOOSER_ACTION_OPEN);

			if(!dialog.get_file_path(filepath))
				return;
		}

		k3d::filesystem::igzstream file(filepath);

		k3d::iscript_engine::context context;
		context["document"] = &document();

		execute_script(file, filepath.native_utf8_string().raw(), context);
	}

	void on_scripting_script_editor()
	{
		if(Gtk::Window* const window = k3d::plugin::create<Gtk::Window>("NGUITextEditorDialog"))
		{
			if(k3d::idocument_sink* const document_sink = dynamic_cast<k3d::idocument_sink*>(window))
				document_sink->set_document(&document());

			window->set_transient_for(*this);
		}
	}

	void on_scripting_python_shell()
	{
		if(Gtk::Window* const window = k3d::plugin::create<Gtk::Window>("NGUIPythonShellDialog"))
		{
			if(k3d::idocument_sink* const document_sink = dynamic_cast<k3d::idocument_sink*>(window))
				document_sink->set_document(&document());

			window->set_transient_for(*this);
		}
	}

	void on_scripting_action(k3d::iplugin_factory* Factory)
	{
		boost::scoped_ptr<k3d::iunknown> plugin(k3d::plugin::create(*Factory));
		if(!plugin)
		{
			k3d::log() << error << "Error creating plugin [" << (*Factory).name() << "]" << std::endl;
			return;
		}

		if(k3d::iscripted_action* const scripted_action = dynamic_cast<k3d::iscripted_action*>(plugin.get()))
		{
			k3d::iscript_engine::context context;
			context["command"] = k3d::string_t("action");
			context["document"] = &document();
			scripted_action->execute(context);
		}
	}

	void on_window_fullscreen(k3d::iunknown*)
	{
		if(m_fullscreen.internal_value())
			fullscreen();
		else
			unfullscreen();
	}

	void on_advanced_create_dialog(k3d::iplugin_factory* Factory)
	{
		Gtk::Window* const window = k3d::plugin::create<Gtk::Window>(*Factory);
		return_if_fail(window);

		window->set_transient_for(*this);
	}

	void on_help_learning_menu()
	{
		Gtk::Window* const window = k3d::plugin::create<Gtk::Window>("NGUILearningDialog");
		return_if_fail(window);

		window->set_transient_for(*this);
	}

	void on_help_file_bug_report()
	{
		k3d::ngui::uri::open("http://www.k-3d.org/wiki/Reporting_Bugs");
	}

	void on_help_open_log_window()
	{
		Gtk::Window* const window = k3d::plugin::create<Gtk::Window>("NGUILogDialog");
		return_if_fail(window);

		window->set_transient_for(*this);
	}

	void on_help_guide()
	{
		k3d::ngui::uri::open("file://" + (k3d::share_path() / k3d::filesystem::generic_path("/guide/html/index.html")).native_filesystem_string());
	}

	void on_help_online()
	{
		k3d::ngui::uri::open("http://www.k-3d.org");
	}

	void on_help_release_notes()
	{
		k3d::ngui::uri::open("http://www.k-3d.org/wiki/K-3D_" K3D_VERSION "_Release_Notes");
	}

	void on_help_about()
	{
		Gtk::Window* const window = k3d::plugin::create<Gtk::Window>("NGUIAboutDialog");
		return_if_fail(window);

		window->set_transient_for(*this);
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
		return_if_fail(camera);

		k3d::irender_camera_preview* render_engine = viewport_control->camera_preview_engine();
		if(!render_engine)
			render_engine = pick_camera_preview_render_engine(m_document_state);
		if(!render_engine)
			return;

		viewport_control->set_camera(camera);
		viewport_control->set_camera_preview_engine(render_engine);

		render(*camera, *render_engine);
	}

	void on_render_frame()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::icamera* const camera = viewport_control->camera();
		return_if_fail(camera);

		k3d::irender_camera_frame* render_engine = viewport_control->camera_still_engine();
		if(!render_engine)
			render_engine = pick_camera_still_render_engine(m_document_state);
		if(!render_engine)
			return;

		viewport_control->set_camera(camera);
		viewport_control->set_camera_still_engine(render_engine);

		render(*camera, *render_engine);
	}

	void on_render_animation()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::icamera* const camera = viewport_control->camera();
		return_if_fail(camera);

		k3d::irender_camera_animation* render_engine = viewport_control->camera_animation_engine();
		if(!render_engine)
			render_engine = pick_camera_animation_render_engine(m_document_state);
		if(!render_engine)
			return;

		viewport_control->set_camera(camera);
		viewport_control->set_camera_animation_engine(render_engine);

		render(m_document_state, *camera, *render_engine);
	}

	void on_render_viewport_frame()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::irender_camera_frame* render_engine = dynamic_cast<k3d::irender_camera_frame*>(viewport_control);
		return_if_fail(render_engine);

		k3d::icamera* camera = viewport_control->camera();
		if(!camera)
			camera = pick_camera(m_document_state);
		if(!camera)
			return;

		viewport_control->set_camera(camera);
		render(*camera, *render_engine);
	}

	void on_render_viewport_animation()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::irender_camera_animation* render_engine = dynamic_cast<k3d::irender_camera_animation*>(viewport_control);
		return_if_fail(render_engine);

		k3d::icamera* camera = viewport_control->camera();
		if(!camera)
			camera = pick_camera(m_document_state);
		if(!camera)
			return;

		viewport_control->set_camera(camera);
		render(m_document_state, *camera, *render_engine);
	}

	void on_render_set_viewport_preview_engine()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::irender_camera_preview* const engine = pick_camera_preview_render_engine(m_document_state);
		if(!engine)
			return;

		viewport_control->set_camera_preview_engine(engine);
	}

	void on_render_set_viewport_still_engine()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::irender_camera_frame* const engine = pick_camera_still_render_engine(m_document_state);
		if(!engine)
			return;

		viewport_control->set_camera_still_engine(engine);
	}

	void on_render_set_viewport_animation_engine()
	{
		viewport::control* const viewport_control = m_document_state.get_focus_viewport();
		return_if_fail(viewport_control);

		k3d::irender_camera_animation* const engine = pick_camera_animation_render_engine(m_document_state);
		if(!engine)
			return;

		viewport_control->set_camera_animation_engine(engine);
	}

	void on_document_title_changed(k3d::iunknown*)
	{
		k3d::ustring doctitle = boost::any_cast<k3d::ustring>(document().title().property_internal_value());
		set_title(doctitle.raw() + " - K-3D");
		get_accessible()->set_name(doctitle.raw());
	}

	void on_panel_focus_changed(panel_frame::control* Panel)
	{
		if(m_focus_panel == Panel)
			return;

		m_focus_panel = Panel;

		if(Panel)
		{
			set_focus_viewport_panel(Panel);

			const bool decorated = Panel->decorations.internal_value();
			m_layout_decorate_panel->set_sensitive(!decorated);
			m_layout_undecorate_panel->set_sensitive(decorated);
		}

		update_panel_controls(0);
	}

	void update_panel_controls(k3d::iunknown*)
	{
		unsigned long panel_count = 0;
		unsigned long pinned_count = 0;
		unsigned long visible_count = 0;

		const panel_frame::controls panel_frames = detail::get_panel_frames(m_panel_frame);
		for(panel_frame::controls::const_iterator panel_frame = panel_frames.begin(); panel_frame != panel_frames.end(); ++panel_frame)
		{
			// Ensure that the focus viewport defaults to any available viewport ...
			if(!m_focus_viewport_panel)
				set_focus_viewport_panel(*panel_frame);

			++panel_count;

			if((*panel_frame)->pinned.internal_value())
				++pinned_count;

			if((*panel_frame)->is_visible())
				++visible_count;
		}

		m_layout_pin_all->set_sensitive(panel_count != pinned_count);
		m_layout_unpin_all->set_sensitive(pinned_count);
		m_layout_split_horizontal->set_sensitive(m_focus_panel);
		m_layout_split_vertical->set_sensitive(m_focus_panel);
		m_layout_kill_panel->set_sensitive((panel_count > 1) && m_focus_panel);
	}

	void save_ui_layout()
	{
		k3d::xml::element xml_document("ui_layout");

		// Save window's parameters
		xml_document.append(k3d::xml::attribute("fullscreen", m_fullscreen.internal_value()));
		int width;
		int height;
		get_size(width, height);
		xml_document.append(k3d::xml::attribute("window_width", width));
		xml_document.append(k3d::xml::attribute("window_height", height));
		int root_x;
		int root_y;
		get_position(root_x, root_y);
		xml_document.append(k3d::xml::attribute("position_x", root_x));
		xml_document.append(k3d::xml::attribute("position_y", root_y));

		return_if_fail(m_panel_frame.get_children().size() == 1);
		Gtk::Widget* frame_child = *m_panel_frame.get_children().begin();

		// Save panel frame
		k3d::xml::element& xml_panel_frame = xml_document.append(k3d::xml::element("panel_frame"));
		save_ui_container(frame_child, xml_panel_frame);

		k3d::filesystem::ofstream stream(detail::ui_layout_path());
		stream << xml_document;
	}

	bool load_ui_layout()
	{
		if(!application_state::instance().custom_layouts())
			return false;

		const k3d::filesystem::path layout_path = detail::ui_layout_path();
		if(!k3d::filesystem::exists(layout_path))
			return false;

		k3d::xml::element xml_layout;
		try
		{
			k3d::log() << info << "Loading UI layout from " << layout_path.native_console_string() << std::endl;

			// Find layout
			k3d::filesystem::ifstream layout_stream(layout_path);
			layout_stream >> xml_layout;
			assert_warning(xml_layout.name == "ui_layout");
		}
		catch(...)
		{
			k3d::log() << error << "Error loading UI layout from " << layout_path.native_console_string() << std::endl;
			return false;
		}

		// Set window's values
		const bool fullscreen = k3d::xml::attribute_value<bool>(xml_layout, "fullscreen", false);
		m_fullscreen.set_value(fullscreen);
		const int window_width = k3d::xml::attribute_value<int>(xml_layout, "window_width", 0);
		const int window_height = k3d::xml::attribute_value<int>(xml_layout, "window_height", 0);
		if(window_width && window_height)
			set_default_size(window_width, window_height);
		const int root_x = k3d::xml::attribute_value<int>(xml_layout, "position_x", 0);
		const int root_y = k3d::xml::attribute_value<int>(xml_layout, "position_y", 0);
		move(root_x, root_y);

		// Load panel frame
		k3d::xml::element* xml_panel = find_element(xml_layout, "panel_frame");
		return_val_if_fail(xml_panel, false);

		return load_ui_container(*xml_panel);
	}

	void save_ui_container(Gtk::Widget* Widget, k3d::xml::element& Document)
	{
		Gtk::Paned* gtk_paned = dynamic_cast<Gtk::Paned*>(Widget);
		if(gtk_paned)
		{
			k3d::xml::element& xml_paned = Document.append(k3d::xml::element("paned"));
			std::string paned_type = "";

			if(dynamic_cast<Gtk::HPaned*>(gtk_paned))
				paned_type = "hpaned";
			else if(dynamic_cast<Gtk::VPaned*>(gtk_paned))
				paned_type = "vpaned";
			else
				assert_not_reached();

			xml_paned.append(k3d::xml::attribute("type", paned_type));
			xml_paned.append(k3d::xml::attribute("position", gtk_paned->get_position()));

			save_ui_container(gtk_paned->get_child1(), xml_paned);
			save_ui_container(gtk_paned->get_child2(), xml_paned);

			return;
		}

		panel_frame::control* k3d_panel = dynamic_cast<panel_frame::control*>(Widget);
		if(k3d_panel)
		{
			k3d_panel->save(Document);

			return;
		}

		assert_not_reached();
	}

	// Loads UI, returns false iff main panel could load
	bool load_ui_container(k3d::xml::element& Element)
	{
		k3d::xml::element* xml_panel = find_element(Element, "paned");
		if(!xml_panel)
			xml_panel = find_element(Element, "panel");

		return_val_if_fail(xml_panel, false);

		// Load main panel
		Gtk::Widget* main_panel = load_panel(*xml_panel);
		if(main_panel)
		{
			if(Gtk::Widget* frame_child = m_panel_frame.get_child())
			{
				m_panel_frame.remove();
				delete frame_child;
			}

			m_panel_frame.set_shadow_type(Gtk::SHADOW_NONE);
			m_panel_frame.add(*Gtk::manage(main_panel));

			update_panel_controls(0);

			return true;
		}

		return false;
	}

	Gtk::Widget* load_panel(k3d::xml::element& Paned)
	{
		// Load panel ...
		if("panel" == Paned.name)
		{
			panel_frame::control* const new_panel_frame = create_panel_frame();
			new_panel_frame->load(Paned);

			if(new_panel_frame->is_visible())
				new_panel_frame->show();

			return new_panel_frame;
		}

		// ... else load paned
		return_val_if_fail("paned" == Paned.name, 0);
		return_val_if_fail(Paned.children.size(), 0);

		std::string type = attribute_text(Paned, "type");
		Gtk::Paned* new_paned = 0;
		if("hpaned" == type)
			new_paned = Gtk::manage(new Gtk::HPaned);
		else if("vpaned" == type)
			new_paned = Gtk::manage(new Gtk::VPaned);
		else
		{
			assert_not_reached();
			k3d::log() << error << "Unknowned Gtk::Paned type : " << type << std::endl;
			return 0;
		}
		new_paned->set_position(k3d::xml::attribute_value<int>(Paned, "position", 0));

		// Load paned children
		k3d::xml::element& child1 = *Paned.children.begin();
		new_paned->pack1(*load_panel(child1), Gtk::EXPAND);
		if(new_paned->get_child1()->is_visible())
			new_paned->show();

		if(Paned.children.size() > 1)
		{
			k3d::xml::element& child2 = *(Paned.children.begin() + 1);
			new_paned->pack2(*load_panel(child2), Gtk::EXPAND);
			if(new_paned->get_child2()->is_visible())
				new_paned->show();
		}

		return new_paned;
	}

	bool on_key_press_event(GdkEventKey* event)
	{
		if(event->keyval == GDK_Escape)
		{
			return true;
		}

		return base::on_key_press_event(event);
	}

	bool on_button_press_event(GdkEventButton* event)
	{
		return base::on_button_press_event(event);
	}

	void update_undo_redo_controls()
	{
		k3d::istate_recorder& state_recorder = document().state_recorder();
		const k3d::istate_recorder::node* const current_node = state_recorder.current_node();
		const k3d::istate_recorder::node* const redo_node = next_redo(state_recorder);

		// Setup the undo menu item ...
		if(current_node)
		{
			get_label(*m_undo_menu_item)->set_text(k3d::string_cast(boost::format(_("Undo %1%")) % current_node->label));
			m_undo_menu_item->set_sensitive(true);
		}
		else
		{
			get_label(*m_undo_menu_item)->set_text(_("Can't undo"));
			m_undo_menu_item->set_sensitive(false);
		}

		// Setup the undo-all menu item
		if(current_node && current_node->parent && (current_node->label == current_node->parent->label))
		{
			get_label(*m_undo_all_menu_item)->set_text(k3d::string_cast(boost::format(_("Undo all %1%")) % current_node->label));
			m_undo_all_menu_item->set_sensitive(true);
		}
		else
		{
			get_label(*m_undo_all_menu_item)->set_text(_("Can't undo all"));
			m_undo_all_menu_item->set_sensitive(false);
		}

		// Setup the redo menu items ...
		if(redo_node)
		{
			get_label(*m_redo_menu_item)->set_text(k3d::string_cast(boost::format(_("Redo %1%")) % redo_node->label));
			m_redo_menu_item->set_sensitive(true);
			get_label(*m_redo_all_menu_item)->set_text(k3d::string_cast(boost::format(_("Redo all %1%")) % redo_node->label));
			m_redo_all_menu_item->set_sensitive(true);
		}
		else
		{
			get_label(*m_redo_menu_item)->set_text(_("Can't redo"));
			m_redo_menu_item->set_sensitive(false);
			get_label(*m_redo_all_menu_item)->set_text(_("Can't redo all"));
			m_redo_all_menu_item->set_sensitive(false);
		}
	}

	const bool unsaved_changes()
	{
		return document().state_recorder().last_saved_node() != document().state_recorder().current_node();
	}

	const std::string unsaved_document_title()
	{
		return boost::any_cast<k3d::ustring>(document().title().property_internal_value()).raw();
	}

	const bool save_unsaved_changes()
	{
		return on_file_save();
	}

	// Saves focus viewport and updates document_state
	void set_focus_viewport_panel(panel_frame::control* Panel)
	{
		viewport::control* const viewport_control = dynamic_cast<viewport::control*>(Panel->mounted_panel());
		if(!viewport_control)
			return;

		m_focus_viewport_panel = Panel;
		m_document_state.set_focus_viewport(viewport_control);

		// Highlight focused viewport
		const panel_frame::controls panel_frames = detail::get_panel_frames(m_panel_frame);
		for(panel_frame::controls::const_iterator panel_frame = panel_frames.begin(); panel_frame != panel_frames.end(); ++panel_frame)
		{
			if(!dynamic_cast<viewport::control*>((*panel_frame)->mounted_panel()))
				continue;

			if(*panel_frame == Panel)
				(*panel_frame)->set_bg_color(Gdk::Color("blue"));
			else
				(*panel_frame)->unset_bg_color();
		}
	}

	void unset_focus_viewport_panel()
	{
		m_focus_viewport_panel = 0;
		m_document_state.set_focus_viewport(0);
	}

	/// Keeps track of the count of open document windows
	static unsigned long m_count;

	/// Stores the Edit > Undo menu item
	std::unique_ptr<Gtk::ImageMenuItem> m_undo_menu_item;
	/// Stores the Edit > Undo All menu item
	std::unique_ptr<Gtk::ImageMenuItem> m_undo_all_menu_item;
	/// Stores the Edit > Redo menu item
	std::unique_ptr<Gtk::ImageMenuItem> m_redo_menu_item;
	/// Stores the Edit > Redo All menu item
	std::unique_ptr<Gtk::ImageMenuItem> m_redo_all_menu_item;
	/// Set to true iff current panel was maximized
	k3d_data(bool, immutable_name, change_signal, no_undo, local_storage, no_constraint, no_property, no_serialization) m_maximize_panel;
	/// Set to true iff unpinned panels are hidden
	k3d_data(bool, immutable_name, change_signal, no_undo, local_storage, no_constraint, no_property, no_serialization) m_hide_unpinned_panels;
	/// Set to true iff the window is displayed fullscreen
	k3d_data(bool, immutable_name, change_signal, no_undo, local_storage, no_constraint, no_property, no_serialization) m_fullscreen;
	/// Stores a signal for notifying observers when the panel focus changes
	panel_frame::control::panel_focus_signal_t m_panel_focus_signal;
	/// Keeps track of the currently focused panel
	panel_frame::control* m_focus_panel;
	/// Keeps track of the last-focused viewport panel
	panel_frame::control* m_focus_viewport_panel;
	/// Stores the root container for panels
	Gtk::Frame m_panel_frame;

	check_menu_item::control* m_layout_maximize_panel;
	check_menu_item::control* m_layout_hide_unpinned;
	Gtk::MenuItem* m_layout_pin_all;
	Gtk::MenuItem* m_layout_unpin_all;
	Gtk::MenuItem* m_layout_decorate_panel;
	Gtk::MenuItem* m_layout_undecorate_panel;
	Gtk::MenuItem* m_layout_split_horizontal;
	Gtk::MenuItem* m_layout_split_vertical;
	Gtk::MenuItem* m_layout_kill_panel;

	/// Stores document-wide state to be shared with the rest of the UI
	document_state& m_document_state;
};

unsigned long main_document_window::m_count = 0;

/////////////////////////////////////////////////////////////////////////////
// create_main_document_window

void create_main_document_window(document_state& DocumentState)
{
	new main_document_window(DocumentState);
}

} // namespace ngui

} // namespace k3d

