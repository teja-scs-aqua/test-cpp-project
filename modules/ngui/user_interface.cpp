// K-3D
// Copyright (c) 1995-2006, Timothy M. Shead
//
// Contact: tshead@k-3d.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your argument) any later version.
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

#include "file_notification.h"
#include "splash_box.h"

#include <k3d-i18n-config.h>
#include <k3d-version-config.h>
#include <k3dsdk/ngui/application_state.h>
#include <k3dsdk/ngui/application_window.h>
#include <k3dsdk/ngui/document.h>
#include <k3dsdk/ngui/document_state.h>
#include <k3dsdk/ngui/file_chooser_dialog.h>
#include <k3dsdk/ngui/main_document_window.h>
#include <k3dsdk/ngui/messages.h>
#include <k3dsdk/ngui/options.h>
#include <k3dsdk/ngui/uri.h>
#include <k3dsdk/ngui/utility.h>

#include <k3dsdk/application.h>
#include <k3dsdk/application_plugin_factory.h>
#include <k3dsdk/classes.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/data.h>
#include <k3dsdk/iapplication.h>
#include <k3dsdk/idocument_importer.h>
#include <k3dsdk/ifile_change_notifier.h>
#include <k3dsdk/iscript_engine.h>
#include <k3dsdk/iscripted_action.h>
#include <k3dsdk/iuser_interface.h>
#include <k3dsdk/ievent_loop.h>
#include <k3dsdk/log.h>
#include <k3dsdk/module.h>
#include <k3dsdk/property.h>
#include <k3dsdk/result.h>
#include <k3dsdk/share.h>
#include <k3dsdk/string_cast.h>
#include <k3dsdk/system.h>

#include <glibmm/main.h>

#include <gtkmm/accelkey.h>
#include <gtkmm/accelmap.h>
#include <gtkmm/main.h>

#include <gtk/gtkgl.h>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

#include <k3dsdk/fstream.h>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

#include <iostream>

using namespace k3d::ngui;

namespace module
{

namespace ngui
{

namespace detail
{

void handle_error(const k3d::string_t& Message, bool& Quit, bool& Error)
{
	k3d::log() << error << Message << std::endl;

	Quit = true;
	Error = true;
}

const k3d::filesystem::path hotkey_path()
{
	return k3d::system::get_home_directory() / k3d::filesystem::generic_path(".k3d/hotkeys");
}

void setup_default_hotkeys()
{
	Gtk::AccelMap::add_entry("<k3d-document>/actions/create/PolyCube", Gtk::AccelKey("F5").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/create/PolyCylinder", Gtk::AccelKey("F6").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/create/PolyGrid", Gtk::AccelKey("F8").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/create/PolySphere", Gtk::AccelKey("F7").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/delete", Gtk::AccelKey("Delete").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/duplicate", Gtk::AccelKey("d").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/instantiate", Gtk::AccelKey("d").get_key(), Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/redo", Gtk::AccelKey("z").get_key(), Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/tools/move_tool", Gtk::AccelKey("w").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/tools/parent_tool", Gtk::AccelKey("p").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/tools/rotate_tool", Gtk::AccelKey("e").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/tools/scale_tool", Gtk::AccelKey("r").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/tools/select_tool", Gtk::AccelKey("q").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/tools/snap_tool", Gtk::AccelKey("s").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/tools/unparent", Gtk::AccelKey("p").get_key(), Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/edit/undo", Gtk::AccelKey("z").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/file/close", Gtk::AccelKey("w").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/file/new", Gtk::AccelKey("n").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/file/open", Gtk::AccelKey("o").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/file/quit", Gtk::AccelKey("q").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/file/save", Gtk::AccelKey("s").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/file/save_as", Gtk::AccelKey("s").get_key(), Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/help/manual", Gtk::AccelKey("F1").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/layout/fullscreen", Gtk::AccelKey("F11").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/layout/hide_unpinned", Gtk::AccelKey("grave").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/CollapseEdges", Gtk::AccelKey("b").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/ConnectVertices", Gtk::AccelKey("j").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/Delete", Gtk::AccelKey("BackSpace").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/Dissolve", Gtk::AccelKey("BackSpace").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/ExtrudeFaces", Gtk::AccelKey("m").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/FilletEdges", Gtk::AccelKey("n").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/FlipOrientation", Gtk::AccelKey("i").get_key(), Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/GrowSelection", Gtk::AccelKey("equal").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/SelectEdgeLoops", Gtk::AccelKey("semicolon").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/SelectEdgeRings", Gtk::AccelKey("l").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/SubdivideEdges", Gtk::AccelKey("k").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/modifier/SubdivideFaces", Gtk::AccelKey("k").get_key(), Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/render/render_frame", Gtk::AccelKey("F12").get_key(), Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/render/render_preview", Gtk::AccelKey("F12").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/render/render_region", Gtk::AccelKey("F12").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_all", Gtk::AccelKey("a").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_child", Gtk::AccelKey("bracketleft").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_faces", Gtk::AccelKey("4").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_invert", Gtk::AccelKey("i").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_lines", Gtk::AccelKey("3").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_nodes", Gtk::AccelKey("1").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_none", Gtk::AccelKey("space").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_parent", Gtk::AccelKey("p").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_points", Gtk::AccelKey("2").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/select/select_sibling", Gtk::AccelKey("bracketright").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/aim_selection", Gtk::AccelKey("a").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/assign_hotkeys", Gtk::AccelKey("Insert").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/frame_selection", Gtk::AccelKey("f").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/hide_selection", Gtk::AccelKey("h").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/hide_unselected", Gtk::AccelKey("h").get_key(), Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/set_camera", Gtk::AccelKey("c").get_key(), Gdk::ModifierType(0));
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/show_all", Gtk::AccelKey("h").get_key(), Gdk::CONTROL_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/show_selection", Gtk::AccelKey("h").get_key(), Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
	Gtk::AccelMap::add_entry("<k3d-document>/actions/view/toggle_projection", Gtk::AccelKey("o").get_key(), Gdk::ModifierType(0));
}

} // namespace detail

/////////////////////////////////////////////////////////////////////////////
// user_interface

class user_interface :
	public k3d::iuser_interface,
	public k3d::ievent_loop,
	public sigc::trackable
{
public:
	user_interface() :
		m_show_learning_menu(options::nag("show_learning_menu"))
	{
		// This ensures that we can use ATK for testing, even when the user hasn't enabled desktop accessibility.
		k3d::system::setenv("GTK_MODULES", "gail");

		/// Redirect glib-based logging to our own standard logging mechanism
		g_log_set_handler("", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("Gdk", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("GdkPixbuf", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("Glib", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("Glib-GObject", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("GLib-GObject", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("Gtk", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("Pango", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("atk-bridge", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("atkmm", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("gdkmm", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("glibmm", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("gtkmm", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
		g_log_set_handler("pangomm", static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler, 0);
	}

	~user_interface()
	{
		delete_auto_start_plugins();

		const k3d::filesystem::path hotkey_path = detail::hotkey_path();
		k3d::filesystem::create_directories(hotkey_path.branch_path());
		k3d::log() << info << "Saving hotkeys to " << hotkey_path.native_console_string() << std::endl;
		Gtk::AccelMap::save(hotkey_path.native_filesystem_string());
	}

	static void log_handler(const gchar* LogDomain, GLogLevelFlags LogLevel, const gchar* Message, gpointer UserData)
	{
		const k3d::string_t domain = LogDomain ? LogDomain : "";
		k3d::string_t message(Message ? Message : "");
		std::replace(message.begin(), message.end(), '\n', ' ');
		std::replace(message.begin(), message.end(), '\r', ' ');

		switch(LogLevel)
		{
			case G_LOG_LEVEL_ERROR:
				k3d::log() << critical << domain << ": " << message << std::endl;
				break;
			case G_LOG_LEVEL_CRITICAL:
				k3d::log() << error << domain << ": " << message << std::endl;
				break;
			case G_LOG_LEVEL_WARNING:
				k3d::log() << warning << domain << ": " << message << std::endl;
				break;
			case G_LOG_LEVEL_MESSAGE:
			case G_LOG_LEVEL_INFO:
				k3d::log() << info << domain << ": " << message << std::endl;
				break;
			case G_LOG_LEVEL_DEBUG:
				k3d::log() << debug << domain << ": " << message << std::endl;
				break;
			default:
				k3d::log() << domain << ": " << message << std::endl;
				break;
		}
	}

	void get_command_line_arguments(boost::program_options::options_description& Description)
	{
		Description.add_options()
			("new,n", "Create a new document.")
			("no-custom-layouts", "Disable custom user interface layouts (useful when playing-back recorded tutorials).")
			("no-splash", "Disables the startup splash screen.")
			("open,o", boost::program_options::value<k3d::string_t>(), "Open an existing document.")
			("show-learning-menu", "Display the learning menu after startup.")
			;
	}

	const arguments_t parse_startup_arguments(const arguments_t& Arguments, bool& Quit, bool& Error)
	{
		// Keep track of whether to display a splash screen or not ...
		bool show_splash = true;

		// We want GTK to parse an optional RC file from the user's home directory ...
		const k3d::filesystem::path rc_file = k3d::system::get_home_directory() / k3d::filesystem::generic_path(".k3d/gtkrc");
		k3d::filesystem::create_directories(rc_file.branch_path());
		if(!k3d::filesystem::exists(rc_file))
		{
			k3d::filesystem::ofstream stream(rc_file);
			stream << "# You can add your own K-3D-specific GTK styles here.\n\n";

			stream << "# Un-comment this to modify icon sizes.\n";
			stream << "# gtk-icon-sizes = \"mini-commander-icon=24,24:print-manager=64,64:panel-button=32,32:gtk-dnd=48,48:gtk-menu=32,32:panel-menu=48,48:gtk-large-toolbar=48,48:gtk-small-toolbar=32,32:gtk-button=32,32:gtk-dialog=64,64\"\n\n";

			stream << "# Add styles that will apply to all K-3D widgets here.\n";
			stream << "style \"k3d-style\"\n";
			stream << "{\n\n";

			stream << "}\n\n";

			stream << "class \"*\" style \"k3d-style\"\n";
		}
		if(k3d::filesystem::exists(rc_file))
		{
			Gtk::RC::add_default_file(rc_file.native_filesystem_string());
		}
		else
		{
			k3d::log() << warning << "Could not locate or create " << rc_file.native_console_string() << " resource file" << std::endl;
		}

		try
		{
			const Glib::StringArrayHandle default_files = Gtk::RC::get_default_files();
			for(Glib::StringArrayHandle::const_iterator file = default_files.begin(); file != default_files.end(); ++file)
				k3d::log() << info << "Loading GTK resources from " << *file << std::endl;
		}
		catch(...)
		{
			/** \todo Figure-out why printing a path with a non-ASCII character throws an exception,
			see http://sourceforge.net/tracker/index.php?func=detail&aid=1364098&group_id=11113&atid=111113 */
			k3d::log() << error << "Caught exception logging GTK resource paths" << std::endl;
		}

		// gtkmm expects to parse and modify argc / argv, so create some temporary storage for it to fiddle with ...
		std::vector<char*> argv_buffer;
		argv_buffer.push_back(const_cast<char*>("k3d"));
//		for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
//			argv_buffer.push_back(const_cast<char*>(argument->c_str()));
		int argc = argv_buffer.size();
		char** argv = &argv_buffer[0];

		m_main.reset(new Gtk::Main(argc, argv));

		// Give gtkglext a chance at the startup arguments ...
		if(!gtk_gl_init_check(&argc, &argv))
		{
			detail::handle_error("Could not initialize gtkglext", Quit, Error);
			return arguments_t();
		}

		// We return any "unused" arguments ...
		arguments_t unused;

		// For each command-line argument ...
		for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
		{
			if(argument->string_key == "no-custom-layouts")
			{
				application_state::instance().enable_custom_layouts(false);
			}
			else if(argument->string_key == "no-splash")
			{
				show_splash = false;
			}
			else if(argument->string_key == "show-learning-menu")
			{
				m_show_learning_menu = true;
			}
			else
			{
				unused.push_back(*argument);
			}
		}

		Gtk::Window::set_default_icon(load_pixbuf(k3d::share_path() / k3d::filesystem::generic_path("ngui/pixmap"), k3d::filesystem::generic_path("default_icon.png")));

		if(show_splash)
			m_splash_box.reset(new splash_box(k3d::share_path() / k3d::filesystem::generic_path("ngui/pixmap")));

		return unused;
	}

	void startup_message_handler(const k3d::string_t& Message)
	{
		if(m_splash_box.get())
			m_splash_box->on_startup_message(Message);
	}

	void display_user_interface()
	{
    g_type_init();
		detail::setup_default_hotkeys();

		const k3d::filesystem::path hotkey_path = detail::hotkey_path();
		k3d::log() << info << "Loading hotkeys from " << hotkey_path.native_console_string() << std::endl;
		Gtk::AccelMap::load(hotkey_path.native_filesystem_string());

		m_splash_box.reset();

	        create_document();

		if(m_show_learning_menu)
		{
			Gtk::Window* const window = k3d::plugin::create<Gtk::Window>("NGUILearningDialog");
			if(!window)
				k3d::log() << error << "Error creating learning dialog at startup" << std::endl;
		}

		create_auto_start_plugins();
	}

	const arguments_t parse_runtime_arguments(const arguments_t& Arguments, bool& Quit, bool& Error)
	{
		// We return any "unused" arguments ...
		arguments_t unused;

		// For each command-line argument ...
		for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
		{
			if(argument->string_key == "new")
			{
			    create_document();
			}
			else if(argument->string_key == "open")
			{
				if(argument->value.size() != 1 || argument->value[0].empty())
				{
					detail::handle_error("You must supply a file path with the --open argument!", Quit, Error);
					return arguments_t();
				}

				open_document(k3d::filesystem::native_path(k3d::ustring::from_utf8(argument->value[0])));
			}
			else if(argument->position_key != -1)
			{
				open_document(k3d::filesystem::native_path(k3d::ustring::from_utf8(argument->value[0])));
			}
			else
			{
				unused.push_back(*argument);
			}
		}

		return unused;
	}

	void start_event_loop()
	{
		get_file_notification()->start();
		m_main->run();
	}

	void stop_event_loop()
	{
		get_file_notification()->stop();
		m_main->quit();
	}

	void open_uri(const k3d::string_t& URI)
	{
		k3d::ngui::uri::open(URI);
	}

	void message(const k3d::string_t& Message)
	{
		k3d::ngui::message(Message);
	}

	void warning_message(const k3d::string_t& Message)
	{
		k3d::ngui::warning_message(Message);
	}

	void error_message(const k3d::string_t& Message)
	{
		k3d::ngui::error_message(Message);
	}

	k3d::uint_t query_message(const k3d::string_t& Message, const k3d::uint_t DefaultOption, const std::vector<k3d::string_t>& Options)
	{
		return k3d::ngui::query_message(Message, DefaultOption, Options);
	}

	void nag_message(const k3d::string_t& Type, const k3d::ustring& Message, const k3d::ustring& SecondaryMessage)
	{
		k3d::ngui::nag_message(Type, Message, SecondaryMessage);
	}

	k3d::bool_t get_file_path(const k3d::ipath_property::mode_t Mode, const k3d::string_t& Type, const k3d::string_t& Prompt, const k3d::filesystem::path& OldPath, k3d::filesystem::path& Result)
	{
		file_chooser_dialog dialog(Prompt, Type, Mode);
		return dialog.get_file_path(Result);
	}

	k3d::bool_t show(iunknown& Object)
	{
		k3d::log() << error << k3d_file_reference << ": not implemented!" << std::endl;
		return false;
	}

	void synchronize()
	{
		k3d::ngui::handle_pending_events();
	}

	sigc::connection get_timer(const k3d::double_t FrameRate, sigc::slot<void> Slot)
	{
		return_val_if_fail(FrameRate != 0.0, sigc::connection());

		const unsigned int interval = static_cast<unsigned int>(1000.0 / FrameRate);
		return Glib::signal_timeout().connect(sigc::bind_return(Slot, true), interval);
	}

	k3d::uint_t watch_path(const k3d::filesystem::path& Path, const sigc::slot<void>& Slot)
	{
		return get_file_notification()->watch_path(Path, Slot);
	}
	void unwatch_path(const k3d::uint_t WatchID)
	{
		get_file_notification()->unwatch_path(WatchID);
	}

	k3d::iplugin_factory& factory()
	{
		return get_factory();
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::application_plugin_factory<user_interface,
			k3d::interface_list<k3d::ievent_loop> > factory(
				k3d::uuid(0x444fbabf, 0x08164c85, 0x879751e7, 0x2d6d05b5),
				"NGUI",
				"Next Generation User Interface (NGUI)",
				"Interface");

		return factory;
	}

private:
	void create_auto_start_plugins()
	{
		const k3d::plugin::factory::collection_t factories = k3d::plugin::factory::lookup();
		for(k3d::plugin::factory::collection_t::const_iterator factory = factories.begin(); factory != factories.end(); ++factory)
		{
			k3d::iplugin_factory::metadata_t metadata = (**factory).metadata();

			if(!metadata.count("ngui:application-start"))
				continue;

			k3d::log() << info << "Creating plugin [" << (**factory).name() << "] via ngui:application-start" << std::endl;

			k3d::iunknown* const plugin = k3d::plugin::create(**factory);
			if(!plugin)
			{
				k3d::log() << error << "Error creating plugin [" << (**factory).name() << "] via ngui:application-start" << std::endl;
				continue;
			}
			m_auto_start_plugins.push_back(plugin);

			if(k3d::iscripted_action* const scripted_action = dynamic_cast<k3d::iscripted_action*>(plugin))
			{
				k3d::iscript_engine::context context;
				context["command"] = k3d::string_t("startup");
				scripted_action->execute(context);
			}
		}
	}

	void delete_auto_start_plugins()
	{
		for(auto_start_plugins_t::iterator plugin = m_auto_start_plugins.begin(); plugin != m_auto_start_plugins.end(); ++plugin)
		{
			if(k3d::iscripted_action* const scripted_action = dynamic_cast<k3d::iscripted_action*>(*plugin))
			{
				k3d::iscript_engine::context context;
				context["command"] = k3d::string_t("shutdown");
				scripted_action->execute(context);
			}
		}

		for(auto_start_plugins_t::iterator plugin = m_auto_start_plugins.begin(); plugin != m_auto_start_plugins.end(); ++plugin)
			delete *plugin;

		m_auto_start_plugins.clear();
	}
	
	/// Handles file-notification functionality
	file_notification* get_file_notification()
	{
		static file_notification* file_notifier = 0;
		if(!file_notifier)
			file_notifier = new file_notification();
		return file_notifier;
	}
	
	/// Set to true iff we should display the tutorial menu at startup
	k3d::bool_t m_show_learning_menu;
	/// Stores the main loop
	std::unique_ptr<Gtk::Main> m_main;
	/// Stores the (optional) splash screen
	std::unique_ptr<splash_box> m_splash_box;

	/// Stores (optional) auto-start plugins
	typedef std::vector<k3d::iunknown*> auto_start_plugins_t;
	/// Stores (optional) auto-start plugins
	auto_start_plugins_t m_auto_start_plugins;
};
	
/////////////////////////////////////////////////////////////////////////////
// user_interface_factory

k3d::iplugin_factory& user_interface_factory()
{
	return user_interface::get_factory();
}

} // namespace ngui

} // namespace module

K3D_MODULE_START(Registry)
	Registry.register_factory(module::ngui::user_interface_factory());
K3D_MODULE_END

