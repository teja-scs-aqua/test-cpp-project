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

#include <k3dsdk/ngui/toggle_button.h>
#include <k3dsdk/ngui/utility.h>

#include <k3dsdk/istate_recorder.h>
#include <k3dsdk/state_change_set.h>

#include <gtkmm/tooltips.h>

namespace k3d
{

namespace ngui
{

namespace toggle_button
{

/*
/// Specialization of k3d::toggle_button::data_proxy for use with k3d::iproperty objects
template<>
class data_proxy<k3d::iproperty> :
	public idata_proxy
{
public:
	typedef k3d::iproperty data_t;

	data_proxy(data_t& Data, k3d::istate_recorder* const StateRecorder, const Glib::ustring& ChangeMessage) :
		idata_proxy(StateRecorder, ChangeMessage),
		m_readable_data(Data),
		m_writable_data(dynamic_cast<k3d::iwritable_property*>(&Data))
	{
	}

	bool value()
	{
		return boost::any_cast<bool>(m_readable_data.property_internal_value());
	}

	void set_value(const bool Value)
	{
		return_if_fail(m_writable_data);
		m_writable_data->property_set_value(Value);
	}

	changed_signal_t& changed_signal()
	{
		return m_readable_data.property_changed_signal();
	}

private:
	data_proxy(const data_proxy& RHS);
	data_proxy& operator=(const data_proxy& RHS);

	data_t& m_readable_data;
	k3d::iwritable_property* const m_writable_data;
};

std::unique_ptr<idata_proxy> proxy(k3d::iproperty& Data, k3d::istate_recorder* const StateRecorder, const Glib::ustring& ChangeMessage)
{
	return std::unique_ptr<idata_proxy>(new data_proxy<k3d::iproperty>(Data, StateRecorder, ChangeMessage));
}
*/

/////////////////////////////////////////////////////////////////////////////
// control

control::control(imodel* const Model, k3d::istate_recorder* const StateRecorder) :
	m_model(Model),
	m_state_recorder(StateRecorder)
{
	set_name("k3d-toggle-button");
	attach();
}

control::control(imodel* const Model, k3d::istate_recorder* const StateRecorder, const Glib::ustring& label, bool mnemonic) :
	base(label, mnemonic),
	m_model(Model),
	m_state_recorder(StateRecorder)
{
	set_name("k3d-toggle-button");
	attach();
}

void control::attach()
{
	// Update the display ...
	update();

	// We want to be notified if the data source changes ...
	if(m_model)
		m_model->connect_changed_signal(sigc::mem_fun(*this, &control::update));
}

void control::on_update()
{
}

void control::update()
{
	if(m_model)
	{
		const k3d::bool_t new_value = m_model->value();
		if(new_value != get_active())
			set_active(new_value);
	}

	on_update();
}

void control::on_toggled()
{
	if(m_model)
	{
		// Get the control value ...
		const bool new_value = get_active();

		// If the value hasn't changed, we're done ...
		if(new_value != m_model->value())
		{
			// Turn this into an undo/redo -able event ...
			if(m_state_recorder)
				m_state_recorder->start_recording(k3d::create_state_change_set(K3D_CHANGE_SET_CONTEXT), K3D_CHANGE_SET_CONTEXT);

			// Update everything with the new value ...
			m_model->set_value(new_value);

			// Turn this into an undo/redo -able event ...
			if(m_state_recorder)
				m_state_recorder->commit_change_set(m_state_recorder->stop_recording(K3D_CHANGE_SET_CONTEXT), new_value ? m_model->label() + " \"On\"" : m_model->label() + " \"Off\"", K3D_CHANGE_SET_CONTEXT);
		}
	}
	else
	{
		update();
	}

	base::on_toggled();

	update();
}

} // namespace toggle_button

} // namespace ngui

} // namespace k3d

