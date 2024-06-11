#ifndef K3DSDK_NGUI_POINT3_H
#define K3DSDK_NGUI_POINT3_H

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

#include <k3dsdk/point3.h>
#include <k3dsdk/signal_system.h>

#include <gtkmm/table.h>

#include <memory>

namespace Gtk { class Button; }
namespace k3d { class ihint; }
namespace k3d { class iproperty; }
namespace k3d { class istate_recorder; }

namespace k3d
{

namespace ngui
{

namespace point
{

/////////////////////////////////////////////////////////////////////////////
// idata_proxy

/// Abstract interface for an object that proxies a data source for a point control (i.e. the "model" in model-view-controller)
class idata_proxy
{
public:
	virtual ~idata_proxy() {}

	/// Returns true iff the underlying data source is writable
	virtual bool writable() = 0;
	/// Called to return the underlying data value
	virtual const k3d::point3 value() = 0;
	/// Called to set a new data value
	virtual void set_value(const k3d::point3& Value) = 0;
	/// Signal emitted if the underlying data changes
	typedef sigc::signal<void, k3d::ihint*> changed_signal_t;
	/// Signal emitted if the underlying data changes
	virtual changed_signal_t& changed_signal() = 0;

	/// Stores an optional state recorder for recording undo/redo data
	k3d::istate_recorder* const state_recorder;
	/// Stores an optional message for labelling undo/redo state changes
	const Glib::ustring change_message;

protected:
	idata_proxy(k3d::istate_recorder* const StateRecorder, const Glib::ustring& ChangeMessage) :
		state_recorder(StateRecorder),
		change_message(ChangeMessage)
	{
	}

private:
	idata_proxy(const idata_proxy& RHS);
	idata_proxy& operator=(const idata_proxy& RHS);
};

/////////////////////////////////////////////////////////////////////////////
// control

/// Provides a UI for modifying the point of an object
class control :
	public Gtk::Table
{
	typedef Gtk::Table base;

public:
	control(std::unique_ptr<idata_proxy> Data);

private:
	/// Called to reset the object point to the origin
	void on_reset();

	/// Stores a reference to the underlying data object
	std::unique_ptr<idata_proxy> m_data;
	/// Stores the reset button
	Gtk::Button* m_reset_button;
};

/////////////////////////////////////////////////////////////////////////////
// proxy

/// Convenience factory function for creating k3d::spin_button::idata_proxy objects, specialized for k3d::iproperty
std::unique_ptr<idata_proxy> proxy(k3d::iproperty& Data, k3d::istate_recorder* const StateRecorder = 0, const Glib::ustring& ChangeMessage = Glib::ustring());

} // namespace point

} // namespace ngui

} // namespace k3d

#endif // !K3DSDK_NGUI_POINT3_H

