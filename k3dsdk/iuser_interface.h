#ifndef K3DSDK_IUSER_INTERFACE_H
#define K3DSDK_IUSER_INTERFACE_H

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

#include <k3dsdk/ipath_property.h>
#include <k3dsdk/iunknown.h>
#include <k3dsdk/keyboard.h>
#include <k3dsdk/signal_system.h>
#include <k3dsdk/types.h>
#include <k3dsdk/ustring.h>

#include <vector>

namespace k3d
{

namespace filesystem { class path; }

/// Abstract interface to common graphical-user-interface operations for use by objects
class iuser_interface :
	public virtual iunknown
{
public:
	/// Displays a URI in the user's preferred application.
	virtual void open_uri(const string_t& URI) = 0;
	/// Displays an informational message.
	virtual void message(const string_t& Message) = 0;
	/// Displays a warning message.
	virtual void warning_message(const string_t& Message) = 0;
	/// Displays an error message.
	virtual void error_message(const string_t& Message) = 0;
	/**
		 \brief Prompts the user to choose one of several options
		 \param Message text to be displayed
		 \param DefaultOption one-based index of the option that is selected by default.  If DefaultOption is 0, no option is selected by default.
		 \return one-based index of the option selected by the user, or "0" if a choice was not made (e.g. user clicked WM "close" button)
	*/
	virtual uint_t query_message(const string_t& Message, const uint_t DefaultOption, const std::vector<string_t>& Options) = 0;

	/// Displays an informational "nag" message that users can choose to suppress.
	virtual void nag_message(const string_t& Type, const ustring& Message, const ustring& SecondaryMessage = ustring()) = 0;

	/**
		\brief Prompts the user for a filepath, checking for old choices, and storing the current choice for reuse
		\param Prompt message to display in the file selection dialog
		\param OldPath initial file path to display in the file selection dialog
		\param Result returns the chosen file path
		\return true iff the user confirms the file path choice, false if they wish to cancel the pending operation
	*/
	virtual bool_t get_file_path(const ipath_property::mode_t Mode, const string_t& Type, const string_t& Prompt, const filesystem::path& OldPath, filesystem::path& Result) = 0;

	/// Displays the given object using a graphical user interface
	virtual bool_t show(iunknown& Object) = 0;

	/// Runs the user interface loop (if any) until it is synchronized with the current document state
	virtual void synchronize() = 0;

	/// Returns a connection to a signal that will be emitted at the requested frame rate (could return an empty connection, if the UI doesn't support timers)
	virtual sigc::connection get_timer(const double_t FrameRate, sigc::slot<void> Slot) = 0;
	
	/// Call a slot whenever given filesystem path is modified.  Note that we are watching the
	/// path, not an inode, so it isn't an error to specify a path for a nonexistent file.
	/// The slot will be called when a file is created / modified / renamed / deleted at that
	/// location.  Returns a nonzero watch identifier that is used to cancel the watch later-on,
	/// or 0 if there is an error or the implementation does not support path-watching.
	virtual uint_t watch_path(const filesystem::path& Path, const sigc::slot<void>& Slot) = 0;

	/// Stop watching the given path.
	virtual void unwatch_path(const uint_t WatchID) = 0;


protected:
	iuser_interface() {}
	iuser_interface(const iuser_interface&) {}
	iuser_interface& operator = (const iuser_interface&) { return *this; }
	virtual ~iuser_interface() {}
};

} // namespace k3d

#endif // !K3DSDK_IUSER_INTERFACE_H

