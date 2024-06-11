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

#include <config.h>

#include <k3d-i18n-config.h>
#include <k3d-platform-config.h>
#include <k3d-version-config.h>

// Specialized K-3D interfaces for embedding
#include <k3dsdk/application_detail.h>
#include <k3dsdk/batch_mode_init.h>
#include <k3dsdk/plugin_factory_collection.h>
#include <k3dsdk/network_render_farm.h>
#include <k3dsdk/network_render_farm_detail.h>
#include <k3dsdk/user_interface_init.h>

// Standard K-3D interfaces
#include <k3dsdk/algebra.h>
#include <k3dsdk/application.h>
#include <k3dsdk/classes.h>
#include <k3dsdk/fstream.h>
#include <k3dsdk/gl.h>
#include <k3dsdk/gl/extension.h>
#include <k3dsdk/gzstream.h>
#include <k3dsdk/idocument.h>
#include <k3dsdk/idocument_importer.h>
#include <k3dsdk/ievent_loop.h>
#include <k3dsdk/iscripted_action.h>
#include <k3dsdk/iuser_interface.h>
#include <k3dsdk/log.h>
#include <k3dsdk/log_control.h>
#include <k3dsdk/nodes.h>
#include <k3dsdk/options_policy.h>
#include <k3dsdk/parallel/threads.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/property.h>
#include <k3dsdk/register_application.h>
#include <k3dsdk/register_plugin_factories.h>
#include <k3dsdk/scripting.h>
#include <k3dsdk/shader_cache_detail.h>
#include <k3dsdk/share_detail.h>
#include <k3dsdk/string_modifiers.h>
#include <k3dsdk/system.h>
#include <k3dsdk/type_registry.h>
#include <k3dsdk/types.h>
#include <k3dsdk/utility.h>
#include <k3dsdk/xml.h>

#include <boost/scoped_ptr.hpp>

#include <iomanip>
#include <iterator>

#ifdef K3D_API_WIN32
	#include <k3dsdk/win32.h>
#endif // K3D_API_WIN32

namespace
{

/// Defines storage for parsed command-line arguments
typedef std::vector<boost::program_options::basic_option<char> > arguments_t;

/////////////////////////////////////////////////////////////////////////////
// User options specified on the command line

k3d::bool_t g_show_timestamps = false;
k3d::bool_t g_show_process = false;
k3d::bool_t g_syslog = false;
k3d::bool_t g_color_level = false;
k3d::log_level_t g_minimum_log_level = k3d::K3D_LOG_LEVEL_WARNING;

k3d::filesystem::path g_default_ngui_path;
k3d::filesystem::path g_default_nui_path;
k3d::filesystem::path g_default_options_path;
k3d::filesystem::path g_default_pyui_path;
k3d::filesystem::path g_default_qtui_path;
k3d::filesystem::path g_default_shader_cache_path;
k3d::filesystem::path g_default_share_path;
k3d::filesystem::path g_default_tutorials_path;
k3d::filesystem::path g_default_user_interface_path;
k3d::string_t g_default_plugin_paths;

k3d::filesystem::path g_override_locale_path;
k3d::filesystem::path g_options_path;
k3d::filesystem::path g_shader_cache_path;
k3d::filesystem::path g_share_path;
k3d::filesystem::path g_user_interface_path;
k3d::string_t g_plugin_paths;

k3d::ievent_loop* g_user_interface = 0;

/////////////////////////////////////////////////////////////////////////////
// handle_error

void handle_error(const k3d::string_t& Message, k3d::bool_t& Quit, k3d::bool_t& Error)
{
	Quit = true;
	Error = true;

	throw std::runtime_error(Message);
}

/////////////////////////////////////////////////////////////////////////////
// startup_message_handler

/// Called during application startup to display progress to the user
void startup_message_handler(const k3d::string_t& Message)
{
	k3d::log() << info << Message << std::endl;

	if(g_user_interface)
		g_user_interface->startup_message_handler(Message);
}

/////////////////////////////////////////////////////////////////////////////
// exit_request_handler

/// Called when the user wants to shut everything down
k3d::bool_t exit_request_handler()
{
	if(g_user_interface)
		g_user_interface->stop_event_loop();

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// set_default_options

/// Sets-up default user options for various platforms
void set_default_options(k3d::bool_t& Quit, k3d::bool_t& Error)
{
	// Setup default paths based on the location of the executable ...
	const k3d::filesystem::path executable_dir = k3d::system::executable_path().branch_path();
	const k3d::filesystem::path user_dir = k3d::system::get_home_directory() / k3d::filesystem::generic_path(".k3d");

	g_default_ngui_path = executable_dir / k3d::filesystem::generic_path("../" K3D_LIBDIR "/k3d/plugins/k3d-ngui.module");
	g_default_nui_path = executable_dir / k3d::filesystem::generic_path("../" K3D_LIBDIR "/k3d/plugins/k3d-nui.module");
	g_default_options_path = user_dir / k3d::filesystem::generic_path("options.k3d");
	g_default_plugin_paths = (executable_dir / k3d::filesystem::generic_path("../" K3D_LIBDIR "/k3d/plugins")).native_filesystem_string();
	g_default_pyui_path = executable_dir / k3d::filesystem::generic_path("../" K3D_LIBDIR "/k3d/plugins/k3d-pyui.module");
	g_default_qtui_path = executable_dir / k3d::filesystem::generic_path("../" K3D_LIBDIR "/k3d/plugins/k3d-qtui.module");
	g_default_shader_cache_path = user_dir / k3d::filesystem::generic_path("shadercache");
	g_default_share_path = executable_dir / k3d::filesystem::generic_path("../share/k3d");

#ifdef K3D_ENABLE_OSX_BUNDLE
	g_default_user_interface_path = g_default_qtui_path;
#else
	g_default_user_interface_path = g_default_ngui_path;
#endif

	// Setup path options based on the defaults ...
	g_options_path = g_default_options_path;
	g_plugin_paths = g_default_plugin_paths;
	g_shader_cache_path = g_default_shader_cache_path;
	g_share_path = g_default_share_path;
	g_user_interface_path = g_default_user_interface_path;

	// Optionally override paths using environment variables ...
	if(!k3d::system::getenv("K3D_LOCALE_PATH").empty())
		g_override_locale_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(k3d::system::getenv("K3D_LOCALE_PATH")));

	if(!k3d::system::getenv("K3D_OPTIONS_PATH").empty())
		g_options_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(k3d::system::getenv("K3D_OPTIONS_PATH")));

	if(!k3d::system::getenv("K3D_PLUGIN_PATHS").empty())
		g_plugin_paths = k3d::system::getenv("K3D_PLUGIN_PATHS");

	if(!k3d::system::getenv("K3D_SHADER_CACHE_PATH").empty())
		g_shader_cache_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(k3d::system::getenv("K3D_SHADER_CACHE_PATH")));

	if(!k3d::system::getenv("K3D_SHARE_PATH").empty())
		g_share_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(k3d::system::getenv("K3D_SHARE_PATH")));

	if(!k3d::system::getenv("K3D_USER_INTERFACE_PATH").empty())
		g_user_interface_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(k3d::system::getenv("K3D_USER_INTERFACE_PATH")));

#ifdef K3D_API_WIN32
	// Add the executable directory to PATH
	k3d::system::setenv("PATH", executable_dir.native_filesystem_string() + ";" + k3d::system::getenv("PATH"));
#endif // K3D_API_WIN32
}

/////////////////////////////////////////////////////////////////////////////
// parse_log_arguments

/// Looks for command-line arguments that apply to logging (so we can set them up right away)
const arguments_t parse_log_arguments(const arguments_t& Arguments, k3d::bool_t& Quit, k3d::bool_t& Error)
{
	// Keep track of "unused" options ...
	arguments_t unused;

	for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
	{
		if(argument->string_key == "show-timestamps")
		{
			g_show_timestamps = true;
		}
		else if(argument->string_key == "show-process")
		{
			g_show_process = true;
		}
		else if(argument->string_key == "syslog")
		{
			g_syslog = true;
		}
		else if(argument->string_key == "color")
		{
			g_color_level = true;
		}
		else if(argument->string_key == "no-color")
		{
			g_color_level = false;
		}
		else if(argument->string_key == "log-level")
		{
			if(argument->value[0] == "warning")
				g_minimum_log_level = k3d::K3D_LOG_LEVEL_WARNING;
			else if(argument->value[0] == "information")
				g_minimum_log_level = k3d::K3D_LOG_LEVEL_INFO;
			else if(argument->value[0] == "debug")
				g_minimum_log_level = k3d::K3D_LOG_LEVEL_DEBUG;
			else
			{
				handle_error("Valid values for the --log-level option are \"warning\", \"information\", or \"debug\"", Quit, Error);
				return arguments_t();
			}
		}
		else
		{
			unused.push_back(*argument);
		}
	}

	k3d::log_show_timestamps(g_show_timestamps);
	k3d::log_set_tag(g_show_process ? "[k3d]" : k3d::string_t());
	k3d::log_color_level(g_color_level);
	k3d::log_show_level(true);
	k3d::log_syslog(g_syslog);
	k3d::log_minimum_level(g_minimum_log_level);

	return unused;
}

/////////////////////////////////////////////////////////////////////////////
// parse_startup_arguments

/// Handles "normal" command-line arguments that specify initial application state
const arguments_t parse_startup_arguments(const arguments_t& Arguments, k3d::bool_t& Quit, k3d::bool_t& Error)
{
	// We return any "unused" options ...
	arguments_t unused;

	// For each command-line argument ...
	for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
	{
		if(argument->string_key == "batch")
		{
			k3d::set_batch_mode(true);
		}
		else if(argument->string_key == "ui")
		{
			if(argument->value[0] == "nui" || argument->value[0] == "none")
				g_user_interface_path = g_default_nui_path;
			else if(argument->value[0] == "ngui")
				g_user_interface_path = g_default_ngui_path;
			else if(argument->value[0] == "qtui")
				g_user_interface_path = g_default_qtui_path;
			else if(argument->value[0] == "pyui")
				g_user_interface_path = g_default_pyui_path;
			else
				g_user_interface_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(argument->value[0]));
		}
		else if(argument->string_key == "plugins")
		{
			g_plugin_paths = argument->value[0];
			g_plugin_paths = k3d::replace_all("&", g_default_plugin_paths, g_plugin_paths);
		}
		else if(argument->string_key == "shadercache")
		{
			g_shader_cache_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(argument->value[0]));
		}
		else if(argument->string_key == "share")
		{
			g_share_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(argument->value[0]));
		}
		else if(argument->string_key == "options")
		{
			g_options_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(argument->value[0]));
		}
		else if(argument->string_key == "add-path")
		{
#ifdef K3D_API_WIN32
			k3d::system::setenv("PATH", argument->value[0] + ";" + k3d::system::getenv("PATH"));
#else // K3D_API_WIN32
			k3d::system::setenv("PATH", argument->value[0] + ":" + k3d::system::getenv("PATH"));
#endif // !K3D_API_WIN32
		}
		else if(argument->string_key == "disable-gl-extension")
		{
			k3d::gl::extension::disable(argument->value[0]);
		}
		else if(argument->string_key == "enable-gl-extension")
		{
			k3d::gl::extension::enable(argument->value[0]);
		}
		else if(argument->string_key == "setenv")
		{
			k3d::system::setenv(argument->value[0]);
		}
		else if(argument->string_key == "locale")
		{
			g_override_locale_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(argument->value[0]));
		}
		else
		{
			unused.push_back(*argument);
		}
	}

	return unused;
}

/////////////////////////////////////////////////////////////////////////////
// check_dependencies

/// Checks for any runtime resources required to continue program execution
void check_dependencies(k3d::bool_t& Quit, k3d::bool_t& Error)
{
	k3d::log() << info << "package: " << K3D_PACKAGE << std::endl;
	k3d::log() << info << "version: " << K3D_VERSION << std::endl;
	k3d::log() << info << "platform: " << K3D_HOST << std::endl;
	k3d::log() << info << "compiler: " << K3D_COMPILER_NAME << std::endl;
	k3d::log() << info << "build time: " << __DATE__ << " " << __TIME__ << " local" << std::endl;

#ifdef K3D_HAVE_EXPAT
	k3d::log() << info << "xml parser: expat" << std::endl;
#elif defined K3D_HAVE_LIBXML2
	k3d::log() << info << "xml parser: libxml2" << std::endl;
#else
	k3d::log() << warning "xml parser: unknown" << std::endl;
#endif

	k3d::log() << info << "executable: " << k3d::system::executable_path().native_console_string() << std::endl;
	k3d::log() << info << "options file: " << g_options_path.native_console_string() << std::endl;
	k3d::log() << info << "plugin path(s): " << g_plugin_paths << std::endl;
	k3d::log() << info << "shader cache path: " << g_shader_cache_path.native_console_string() << std::endl;
	k3d::log() << info << "share path: " << g_share_path.native_console_string() << std::endl;
	k3d::log() << info << "user interface: " << g_user_interface_path.native_console_string() << std::endl;
	k3d::log() << info << "home directory: " << k3d::system::get_home_directory().native_console_string() << std::endl;
	k3d::log() << info << "temp directory: " << k3d::system::get_temp_directory().native_console_string() << std::endl;

	// The options file must be specified and must exist ...
	if(g_options_path.empty())
	{
		handle_error("Options path must be specified using --options <path>.", Quit, Error);
		return;
	}

	k3d::filesystem::create_directories(g_options_path.branch_path());
	if(!k3d::filesystem::exists(g_options_path.branch_path()))
	{
		handle_error("Options directory [" + g_options_path.branch_path().native_console_string() + "] does not exist and could not be created.", Quit, Error);
		return;
	}

	// Every specified plugin path must exist ...
	const k3d::system::paths_t plugin_paths = k3d::system::decompose_path_list(g_plugin_paths);
	for(k3d::system::paths_t::const_iterator plugin_path = plugin_paths.begin(); plugin_path != plugin_paths.end(); ++plugin_path)
	{
		if(!k3d::filesystem::exists(*plugin_path))
		{
			handle_error("Plugin path [" + plugin_path->native_console_string() + "] does not exist.", Quit, Error);
			return;
		}
	}

	// The shader cache path must be specified, and must already exist or be successfully created ...
	if(g_shader_cache_path.empty())
	{
		handle_error("Shader cache path must be specified using --shadercache <path>.", Quit, Error);
		return;
	}

	k3d::filesystem::create_directories(g_shader_cache_path);
	if(!k3d::filesystem::exists(g_shader_cache_path))
	{
		handle_error("Shader cache path [" + g_shader_cache_path.native_console_string() + "] does not exist and could not be created.", Quit, Error);
		return;
	}

	// The share path must exist ...
	if(!k3d::filesystem::exists(g_share_path))
	{
		handle_error("Share path [" + g_share_path.native_console_string() + "] does not exist.", Quit, Error);
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////
// create_user_interface

/// Instantiates the (optional) user interface plugin
void create_user_interface(k3d::plugin_factory_collection& Plugins, k3d::bool_t& Quit, k3d::bool_t& Error)
{
	const k3d::string_t module_name = g_user_interface_path.native_console_string();
	if(!g_user_interface_path.empty() && !k3d::filesystem::exists(g_user_interface_path))
	{
		handle_error("UI plugin module [" + module_name + "] does not exist", Quit, Error);
		return;
	}
	Plugins.load_module(g_user_interface_path, k3d::plugin_factory_collection::IGNORE_PROXIES);

	if(Plugins.factories().empty())
	{
		handle_error("UI plugin module [" + module_name + "] does not contain any K-3D plugins", Quit, Error);
		return;
	}
	if(Plugins.factories().size() > 1)
	{
		handle_error("UI plugin module [" + module_name + "] contains more than one K-3D plugin", Quit, Error);
		return;
	}

	g_user_interface = k3d::plugin::create<k3d::ievent_loop>(**Plugins.factories().begin());
	if(!g_user_interface)
	{
		handle_error("UI plugin module [" + module_name + "] does not contain a user interface plugin", Quit, Error);
		return;
	}

	if(!dynamic_cast<k3d::iuser_interface*>(g_user_interface))
	{
		delete g_user_interface;
		g_user_interface = 0;
		handle_error("UI plugin module [" + module_name + "] does not implement k3d::iuser_interface", Quit, Error);
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////
// load_modules

/// Loads (statically- or dynamically-linked) plugin modules
void load_modules(k3d::plugin_factory_collection& Plugins, k3d::bool_t& Quit, k3d::bool_t& Error)
{
	Plugins.load_modules(g_plugin_paths, true, k3d::plugin_factory_collection::LOAD_PROXIES);
}

/////////////////////////////////////////////////////////////////////////////
// parse_runtime_arguments

/// Handles any command-line options that run once startup is complete (e.g. running a script / tutorial)
const arguments_t parse_runtime_arguments(const arguments_t& Arguments, k3d::bool_t& Quit, k3d::bool_t& Error, k3d::iapplication& Application)
{
	// Keep track of "unused" arguments ...
	arguments_t unused;

	// Provide a single script context object that will be used by all scripts, so they can share state ...
	k3d::iscript_engine::context context;

	// For each command-line argument ...
	for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
	{
		if(argument->string_key == "script" || argument->string_key == "script-file")
		{
			k3d::bool_t recognized = false;
			k3d::bool_t executed = false;
			k3d::string_t script_name;
			k3d::script::code script_code("");

			if(argument->string_key == "script")
			{
				script_name = "COMMANDLINE";
				script_code = k3d::script::code(argument->value[0]);
			}
			else if(argument->string_key == "script-file")
			{
				if(argument->value[0] == "-")
				{
					script_name = "STDIN";
					script_code = k3d::script::code(std::cin);
				}
				else
				{
					script_name = argument->value[0];
					const k3d::filesystem::path script_path = k3d::filesystem::native_path(k3d::ustring::from_utf8(script_name));
					if(!k3d::filesystem::exists(script_path))
					{
						k3d::log() << error << "Script [" << script_name << "] doesn't exist" << std::endl;
						Quit = true;
						Error = true;
						return arguments_t();
					}

					k3d::log() << info << "Running script [" << script_name << "]" << std::endl;
					k3d::filesystem::igzstream script_stream(script_path);
					script_code = k3d::script::code(script_stream);
				}
			}

			k3d::script::execute(script_code, script_name, context, recognized, executed);

			if(!recognized)
			{
				k3d::log() << error << "Couldn't recognize scripting language for script [ " << script_name << " ]" << std::endl;
				Quit = true;
				Error = true;
				return arguments_t();
			}

			if(!executed)
			{
				k3d::log() << error << "Error executing script [ " << script_name << " ]" << std::endl;
				Quit = true;
				Error = true;
				return arguments_t();
			}
		}
		else if(argument->string_key == "list-gl-extensions")
		{
			const std::set<k3d::string_t> extensions = k3d::gl::extension::list();
			for(std::set<k3d::string_t>::const_iterator extension = extensions.begin(); extension != extensions.end(); ++extension)
				std::cout << *extension << "\n";

			Quit = true;
			return arguments_t();
		}
		else if(argument->string_key == "exit")
		{
			Quit = true;
			return arguments_t();
		}
		else
		{
			unused.push_back(*argument);
		}
	}

	return unused;
}

/////////////////////////////////////////////////////////////////////////////
// check_unused_arguments

/// Handles any arguments that weren't recognized by either the application or the user interface plugin
void check_unused_arguments(const arguments_t& Arguments, k3d::bool_t& Quit, k3d::bool_t& Error)
{
	// If there aren't any leftover arguments, we're done ...
	if(Arguments.empty())
		return;

	k3d::log() << warning << "The following unknown command-line arguments will be ignored: " << std::endl;
	for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
	    k3d::log() << warning << "name: " << argument->string_key << " value: " << argument->value[0] << std::endl;
}

/////////////////////////////////////////////////////////////////////////////
// create_auto_start_plugins

typedef std::vector<k3d::iunknown*> auto_start_plugins_t;

void create_auto_start_plugins(auto_start_plugins_t& Plugins)
{
	const k3d::plugin::factory::collection_t factories = k3d::plugin::factory::lookup();
	for(k3d::plugin::factory::collection_t::const_iterator factory = factories.begin(); factory != factories.end(); ++factory)
	{
		k3d::iplugin_factory::metadata_t metadata = (**factory).metadata();

		if(!metadata.count("k3d:application-start"))
			continue;

		k3d::log() << info << "Creating plugin [" << (**factory).name() << "] via k3d:application-start" << std::endl;

		k3d::iunknown* const plugin = k3d::plugin::create(**factory);
		if(!plugin)
		{
			k3d::log() << error << "Error creating plugin [" << (**factory).name() << "] via k3d:application-start" << std::endl;
			continue;
		}
		Plugins.push_back(plugin);

		if(k3d::iscripted_action* const scripted_action = dynamic_cast<k3d::iscripted_action*>(plugin))
		{
			k3d::iscript_engine::context context;
			context["command"] = k3d::string_t("startup");
			scripted_action->execute(context);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// delete_auto_start_plugins

void delete_auto_start_plugins(auto_start_plugins_t& Plugins)
{
	for(auto_start_plugins_t::iterator plugin = Plugins.begin(); plugin != Plugins.end(); ++plugin)
	{
		if(k3d::iscripted_action* const scripted_action = dynamic_cast<k3d::iscripted_action*>(*plugin))
		{
			k3d::iscript_engine::context context;
			context["command"] = k3d::string_t("shutdown");
			scripted_action->execute(context);
		}
	}

	for(auto_start_plugins_t::iterator plugin = Plugins.begin(); plugin != Plugins.end(); ++plugin)
		delete *plugin;

	Plugins.clear();
}

} // namespace

int k3d_main(std::vector<k3d::string_t> raw_arguments)
{
	// Append extra options from the environment ...
	std::istringstream buffer(k3d::system::getenv("K3D_EXTRA_OPTIONS"));
	std::copy(std::istream_iterator<k3d::string_t>(buffer), std::istream_iterator<k3d::string_t>(), std::back_inserter(raw_arguments));

	try
	{
		k3d::bool_t quit = false;
		k3d::bool_t error = false;

		// Set default values for all user-configurable options ...
		set_default_options(quit, error);
		if(quit)
			return error ? 1 : 0;

		// Parse command-line options ...
		boost::program_options::options_description description("K-3D options");
		description.add_options()
			("add-path", boost::program_options::value<k3d::string_t>(), "Prepend a path to the PATH environment variable at runtime.")
			("batch", "Enable batch (no user intervention) mode.")
			("color", "Color-code log messages based on their level.")
			("disable-gl-extension", boost::program_options::value<k3d::string_t>(), "Disables the given OpenGL extension.")
			("enable-gl-extension", boost::program_options::value<k3d::string_t>(), "Enables the given OpenGL extension.")
			("exit", "Exits the program (useful after running scripts in batch mode.")
			("help,h", "Prints this help message and exits.")
			("list-gl-extensions", "List available OpenGL extensions and exit.")
#ifdef K3D_ENABLE_NLS
			("locale", boost::program_options::value<k3d::string_t>(), "Overrides the path for loading locales")
#endif // K3D_ENABLE_NLS
			("log-level", boost::program_options::value<k3d::string_t>(), "Specifies the minimum message priority to log - valid values are \"warning\", \"information\", \"debug\" [default: warning].")
			("no-color", "Disable color-coding of log messages based on their level.")
			("options", boost::program_options::value<k3d::string_t>(), "Overrides the filepath for storing user options [default: /home/tshead/.k3d/options.k3d].")
			("plugins", boost::program_options::value<k3d::string_t>(), "Overrides the path(s) for loading plugin libraries [default: /usr/local/k3d/lib/k3d].")
			("script,e", boost::program_options::value<k3d::string_t>(), "Executes the given script text after startup.")
			("script-file,f", boost::program_options::value<k3d::string_t>(), "Executes the given script file after startup (use - for stdin).")
			("setenv", boost::program_options::value<k3d::string_t>(), "Set an environment variable using name=value syntax.")
			("shadercache", boost::program_options::value<k3d::string_t>(), "Overrides the path for storing compiled shaders [default: /home/tshead/.k3d/shadercache].")
			("share", boost::program_options::value<k3d::string_t>(), "Overrides the path for loading shared data files [default: /usr/local/k3d/share/k3d].")
			("show-process", "Prints the process name next to log messages.")
			("show-timestamps", "Prints timestamps next to log messages.")
			("syslog", "Logs messages to syslog.")
			("ui,u", boost::program_options::value<k3d::string_t>(), "Specifies the user interface plugin to use - valid values are a plugin path, \"nui\", \"ngui\", \"qtui\", or \"pyui\" [default: qtui].")
			("user-interface-help,H", "Prints user interface help message and exits.")
			("version", "Prints program version information and exits.")
			;

		arguments_t arguments = boost::program_options::command_line_parser(raw_arguments)
			.options(description)
			.allow_unregistered()
			.style(
				boost::program_options::command_line_style::allow_long
				| boost::program_options::command_line_style::allow_short
				| boost::program_options::command_line_style::allow_dash_for_short
				| boost::program_options::command_line_style::long_allow_adjacent
				| boost::program_options::command_line_style::long_allow_next
				| boost::program_options::command_line_style::short_allow_adjacent
				| boost::program_options::command_line_style::short_allow_next
				)
			.run()
			.options;

		// Handle arguments that cause an immediate exit ...
		for(arguments_t::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument)
		{
			if(argument->string_key == "help")
			{
				std::cout << description << "\n";

				return 0;
			}
			else if(argument->string_key == "version")
			{
				std::cout << "K-3D Version " << K3D_VERSION << "\n";
				std::cout << K3D_COPYRIGHT << "  See the AUTHORS file for contributors.\n";
				std::cout << "Licensed under the GNU General Public License.  See the COPYING file for details.\n";
				std::cout << "K-3D home page: http://k3d.sourceforge.net\n";
				std::cout << "Bug reports to: tshead@k-3d.com\n";

				return 0;
			}
		}

		// Setup our logging options before going any further ...
		arguments = parse_log_arguments(arguments, quit, error);
		if(quit)
			return error ? 1 : 0;

		// Look for command-line options that will configure our subsequent behavior ...
		arguments = parse_startup_arguments(arguments, quit, error);
		if(quit)
			return error ? 1 : 0;

		// Make sure we have all resources required to run ...
		check_dependencies(quit, error);
		if(quit)
			return error ? 1 : 0;

		// Initialize parallel processing ...
		k3d::parallel::set_thread_count(k3d::parallel::automatic);

		// Set the shader cache path ...
		k3d::set_shader_cache_path(g_shader_cache_path);

		// Set the share path ...
		k3d::set_share_path(g_share_path);

#ifdef K3D_ENABLE_NLS
		k3d::filesystem::path locale_path = g_share_path / k3d::filesystem::generic_path("locale");
		if(!g_override_locale_path.empty())
			locale_path = g_override_locale_path;

		k3d::log() << info << "locale path: " << locale_path.native_console_string() << std::endl;

		setlocale(LC_ALL, "");
		bindtextdomain("k3d", locale_path.native_filesystem_string().c_str());
		bind_textdomain_codeset("k3d", "UTF-8");
		textdomain("k3d");
#endif // K3D_ENABLE_NLS

		// Load user options ...
		k3d::options::file_storage user_options(g_options_path);
		k3d::options::set_storage(user_options);

		// Handle creation of the required user interface plugin ...
		k3d::plugin_factory_collection ui_plugins;
		ui_plugins.connect_message_signal(sigc::ptr_fun(startup_message_handler));
		create_user_interface(ui_plugins, quit, error);
		if(quit)
			return error ? 1 : 0;
		return_val_if_fail(g_user_interface, 1);

		// Handle more arguments that may cause our immediate exit ...
		for(arguments_t::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument)
		{
			if(argument->string_key == "user-interface-help")
			{
				boost::program_options::options_description description("User interface options");
				g_user_interface->get_command_line_arguments(description);
				std::cout << description << "\n";

				return 0;
			}
		}

		// Register our user interface ...
		k3d::set_user_interface(*dynamic_cast<k3d::iuser_interface*>(g_user_interface));

		// Ensure the user interface gets cleaned-up ...
		boost::scoped_ptr<k3d::ievent_loop> ui_cleanup(g_user_interface);

		// Give the UI a chance to handle command-line arguments ...
		arguments = g_user_interface->parse_startup_arguments(arguments, quit, error);
		if(quit)
			return error ? 1 : 0;

		// Load plugins ...
		k3d::plugin_factory_collection plugins;
		plugins.connect_message_signal(sigc::ptr_fun(startup_message_handler));
		load_modules(plugins, quit, error);
		if(quit)
			return error ? 1 : 0;

		// Setup a render farm ...
		k3d::network_render_farm render_farm(g_options_path);
		k3d::set_network_render_farm(render_farm);

		// Register plugins ...
		k3d::register_plugin_factories(plugins);

		// Create the main application object ...
		k3d::application_implementation application;

		// We want to be notified when the user requests a shutdown ...
		application.connect_exit_signal(sigc::ptr_fun(exit_request_handler));

		// Register it with the library as the global application object ...
		k3d::register_application(application.interface());

		// Switch the UI to its "normal" (post-startup) layout ...
		startup_message_handler(_("Starting user interface"));
		g_user_interface->display_user_interface();

		// Instantiate "auto-start" plugins ...
		auto_start_plugins_t auto_start_plugins;
		create_auto_start_plugins(auto_start_plugins);

		// Let the UI parse arguments now that startup is complete ...
		arguments = g_user_interface->parse_runtime_arguments(arguments, quit, error);
		if(quit)
			return error ? 1 : 0;

	        // Parse any remaining arguments now that startup is complete ...
		arguments = parse_runtime_arguments(arguments, quit, error, application.interface());
		if(quit)
			return error ? 1 : 0;

		// Check for "unused" command-line arguments ...
		check_unused_arguments(arguments, quit, error);
		if(quit)
			return error ? 1 : 0;

		// Main UI event-loop ...
		g_user_interface->start_event_loop();

		// Cleanup "auto-start" plugins ...
		delete_auto_start_plugins(auto_start_plugins);

		//g_user_interface.reset();

		return 0;
	}
	catch(std::exception& e)
	{
		k3d::string_t message = "Caught exception [";
			message += k3d::demangle(typeid(e));
			message += "] - ";
			message += e.what();

		k3d::log() << critical << message << std::endl;

#ifdef K3D_API_WIN32
		MessageBox(0, message.c_str(), _("K-3D Fatal Error"), MB_OK | MB_ICONSTOP);
#endif // K3D_API_WIN32

		return 1;
	}
	catch(...)
	{
		const k3d::string_t message = "Caught unknown exception.";

		k3d::log() << critical << message << std::endl;

#ifdef K3D_API_WIN32
		MessageBox(0, message.c_str(), _("K-3D Fatal Error"), MB_OK | MB_ICONSTOP);
#endif // K3D_API_WIN32

		return 1;
	}
}

