// K-3D
// Copyright (c) 1995-2010, Timothy M. Shead
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
#include <k3dsdk/fstream.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/qtui/convert.h>
#include <k3dsdk/qtui/message.h>
#include <k3dsdk/qtui/script.h>
#include <k3dsdk/result.h>
#include <k3dsdk/scripting.h>
#include <k3dsdk/string_cast.h>

#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

#include <stack>

#include <QObject>

namespace k3d
{

namespace qtui
{

namespace script
{

namespace detail
{

/*
/// Keeps track of running script engines, so they can be halted if the user hits "escape"
std::stack<k3d::iscript_engine*> script_engine_stack;

/// Halts any running scripts if the user hits "escape"
int script_escape_handler(Gtk::Widget* Widget, GdkEventKey* Event)
{
	if(Event->type != GDK_KEY_RELEASE)
		return false;

	if(Event->keyval != GDK_Escape)
		return false;

	return_val_if_fail(script_engine_stack.size(), false);

	std::vector<string_t> buttons;
	buttons.push_back(_("Yes"));
	buttons.push_back(_("No"));
	if(1 == query_message(_("Halt running script?"), 2, buttons))
		script_engine_stack.top()->halt();

	return true;
}
*/

/// Executes a script using the given plugin factory to create the script engine
bool_t execute(const k3d::script::code& Script, const string_t& ScriptName, k3d::iscript_engine::context& Context, const k3d::script::language& Language)
{
	// Sanity checks ...
	return_val_if_fail(ScriptName.size(), false);

	if(!Language.factory())
	{
		error_message(
			QObject::tr("Could not determine scripting language.  K-3D supports multiple scripting languages, but the language for this script was "
			"not recognized. Most K-3D script engines use some type of \"magic token\" at the beginning of a script to recognize it, e.g. \"#python\" "
			"in the first 7 characters of a script for K-3D's Python engine.  If you are writing a K-3D script, check the documentation "
			"for the scripting language you're writing in to see how to make it recognizable."), "");
		return false;
	}

	// Get the requested scripting engine ...
	boost::scoped_ptr<k3d::iscript_engine> engine(k3d::plugin::create<k3d::iscript_engine>(*Language.factory()));
	if(!engine)
	{
		error_message(
			QObject::tr("Error creating the scripting engine to run this script.  Usually this means that your system is missing appropriate libraries "
			"or that there was an error in installation."), "");
		return false;
	}

/*
	// Intercept global key events ...
	script_engine_stack.push(engine.get());
	sigc::connection script_escape_handler_connection = Gtk::Main::signal_key_snooper().connect(sigc::ptr_fun(script_escape_handler));
*/

	// Run that bad-boy ...
	const bool_t result = engine->execute(ScriptName, Script.source(), Context);

/*
	script_escape_handler_connection.disconnect();
	script_engine_stack.pop();
*/

	if(!result)
	{
		error_message(
			QObject::tr("Error executing script"), "");
	}

	return result;
}

} // namespace detail

bool_t execute(const k3d::script::code& Script, const string_t& ScriptName, k3d::iscript_engine::context& Context, const k3d::script::language& Language)
{
	return detail::execute(Script, ScriptName, Context, Language);
}

bool_t execute(const k3d::script::code& Script, const string_t& ScriptName, k3d::iscript_engine::context& Context)
{
	return detail::execute(Script, ScriptName, Context, k3d::script::language(Script));
}

bool_t execute(const k3d::filesystem::path& Script, k3d::iscript_engine::context& Context)
{
	if(!k3d::filesystem::exists(Script))
	{
		error_message(
			QObject::tr("Requested script file %1 doesn't exist.").arg(k3d::convert<QString>(Script.native_utf8_string())), "");
		return false;
	}

	k3d::filesystem::ifstream file(Script);
	const k3d::script::code script(file);
	const k3d::script::language language(script);

	return detail::execute(script, Script.native_utf8_string().raw(), Context, language);
}

} // namespace script

} // namespace qtui

} // namespace k3d

