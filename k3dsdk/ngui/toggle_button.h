#ifndef K3DSDK_NGUI_TOGGLE_BUTTON_H
#define K3DSDK_NGUI_TOGGLE_BUTTON_H

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

#include <k3dsdk/types.h>

#include <gtkmm/togglebutton.h>

namespace k3d { class istate_recorder; }

namespace k3d
{

namespace ngui
{

namespace toggle_button
{

/////////////////////////////////////////////////////////////////////////////
// imodel

/// Abstract data model for toggle_button::control objects
class imodel
{
public:
	virtual ~imodel() {}

	/// Returns a human-readable label for the underlying data
	virtual const Glib::ustring label() = 0;
	/// Called to return the underlying data value
	virtual const k3d::bool_t value() = 0;
	/// Called to set a new data value
	virtual void set_value(const k3d::bool_t Value) = 0;
	/// Connects a slot to a signal that will be emitted if the underlying data changes
	virtual sigc::connection connect_changed_signal(const sigc::slot<void>& Slot) = 0;

protected:
	imodel() {}

private:
	imodel(const imodel&);
	imodel& operator=(const imodel&);
};

/// Provides an implementation of k3d::toggle_button::imodel that can "wrap" any data source that supports the value(), set_value(), and changed_signal() concepts
template<typename data_t>
class generic_model_t :
	public imodel
{
public:
	generic_model_t(data_t& Data, const Glib::ustring& Label) :
		m_data(Data),
		m_label(Label)
	{
	}

	const Glib::ustring label()
	{
		return m_label;
	}

	const k3d::bool_t value()
	{
		return m_data.internal_value();
	}

	void set_value(const k3d::bool_t Value)
	{
		m_data.set_value(Value);
	}

	sigc::connection connect_changed_signal(const sigc::slot<void>& Slot)
	{
		return m_data.changed_signal().connect(sigc::hide(Slot));
	}

private:
	data_t& m_data;
	const Glib::ustring m_label;
};

/// Convenience factory function for creating generic_model_t objects
template<typename data_t>
imodel* model(data_t& Data, const Glib::ustring& Label = "")
{
	return new generic_model_t<data_t>(Data, Label);
}

/////////////////////////////////////////////////////////////////////////////
// control

/// Provides a UI for manipulating boolean quantities (i.e. the view and the controller from model-view-controller)
class control :
	public Gtk::ToggleButton
{
	typedef Gtk::ToggleButton base;

public:
	control(imodel* const Model, k3d::istate_recorder* const StateRecorder);
	control(imodel* const Model, k3d::istate_recorder* const StateRecorder, const Glib::ustring& label, bool mnemonic = false);

	void on_toggled();

private:
	/// Called to update the state of the widget when the underlying data source changes
	virtual void on_update();

	/// Common construction code
	void attach();
	/// Called when the underlying data source changes
	void update();
	/// Storeas a reference to the underlying data object
	imodel* const m_model;
	k3d::istate_recorder* const m_state_recorder;
};

} // namespace toggle_button

} // namespace ngui

} // namespace k3d

#endif // !K3DSDK_NGUI_TOGGLE_BUTTON_H

