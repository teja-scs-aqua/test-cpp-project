#ifndef K3DSDK_ISCRIPT_ENGINE_H
#define K3DSDK_ISCRIPT_ENGINE_H

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

#include <k3dsdk/iunknown.h>
#include <k3dsdk/signal_system.h>
#include <k3dsdk/types.h>

#include <boost/any.hpp>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace k3d
{

class iplugin_factory;

/// Abstract interface implemented by objects that can execute scripts written in a specific scripting language
/** \note: The Script arguments to execute() have bounced back-and-forth between
 * string and stream representations several times.  The original rationale for making them streams was
 * to avoid having to buffer the source code to a script in memory; the new rationale for making them strings
 * is that in practice you always end-up having to buffer the source code anyway, for one of two reasons:
 * first, very few script APIs are likely to support C++ streams
 * C++ streams, so we will likely have to buffer the source before using it anyway; second, determining
 * the MIME type of a script and calling execute() in sequence (the most common use-case) requires random access to the source stream,
 * which isn't available for compressed streams or socket-based streams.
*/

class iscript_engine :
	public virtual iunknown
{
public:
	virtual ~iscript_engine() {}

	/// Returns a reference to the factory that created this object
	virtual iplugin_factory& factory() = 0;
	/**	\brief Returns the human-readable name of the scripting language this engine implements
	*/
	virtual const string_t language() = 0;

	/// Defines a collection of name-value pairs passed to a script that define its context (its execution environment) - how they are used is implementation-dependent (note that the names are merely suggestions, and may be changed or ignored at the whim of the implementation)
	class context :
		public std::map<string_t, boost::any>
	{
	};

	/// Defines a slot that can be called to redirect script output.
	typedef sigc::slot<void, const string_t&> output_t;

	/**	\brief Executes a script
		\param ScriptName A human readable identifier for the script, which should be used in error messages, etc.
		\param Script The complete source code of the script to be executed
		\param Context Collection of objects that define the context/execution environment of the script -  how they are used is implementation-dependent.  Note that the script engine may alter context objects before returning.
		\param Stdout Optional slot that will be called with script output.
		\param Stderr Optional slot that will be called with script output.
		\return true, iff the script was successfully executed without errors (either syntax or runtime)
	*/
	virtual bool_t execute(const string_t& ScriptName, const string_t& Script, context& Context, output_t* Stdout = 0, output_t* Stderr = 0) = 0;

	/**	\brief Requests a cancellation of all running scripts.
		\return true, iff script cancellation is supported by this engine.
		\note Cancellation may be asynchronous, i.e. scripts may still be running when the call returns, and may continue to run for an indefinite period before shutting down, if at all.
	*/
	virtual bool_t halt() = 0;

protected:
	iscript_engine() {}
	iscript_engine(const iscript_engine& Other) : iunknown(Other) {}
	iscript_engine& operator = (const iscript_engine&) { return *this; }
};

} // namespace k3d

#endif // !K3DSDK_ISCRIPT_ENGINE_H

