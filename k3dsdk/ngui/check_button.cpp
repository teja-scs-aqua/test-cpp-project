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

#include <k3dsdk/ngui/check_button.h>
#include <k3dsdk/ngui/utility.h>

#include <k3dsdk/istate_recorder.h>
#include <k3dsdk/state_change_set.h>

#include <gtkmm/tooltips.h>

namespace k3d
{

namespace ngui
{

namespace check_button
{

/// Specialization of k3d::check_button::data_proxy for use with k3d::iproperty objects
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

	const bool writable()
	{
		return m_writable_data ? true : false;
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

/////////////////////////////////////////////////////////////////////////////
// control

control::control(std::unique_ptr<idata_proxy> Data) :
	m_data(std::move(Data))
{
	set_name("k3d-check-button");
	attach();

	set_sensitive(m_data.get() && m_data->writable());
}

control::control(std::unique_ptr<idata_proxy> Data, const Glib::ustring& label, bool mnemonic) :
	base(label, mnemonic),
	m_data(std::move(Data))
{
	set_name("k3d-check-button");
	attach();

	set_sensitive(m_data.get() && m_data->writable());
}

void control::update(k3d::ihint*)
{
	if(!m_data.get())
		return;

	const bool new_value = m_data->value();
	if(new_value != get_active())
		set_active(new_value);
}

void control::attach()
{
	// Update the display ...
	update(0);

	// We want to be notified if the data source changes ...
	if(m_data.get())
		m_data->changed_signal().connect(sigc::mem_fun(*this, &control::update));
}

void control::on_toggled()
{
	if(m_data.get())
	{
		// If the value hasn't changed, we're done ...
		const bool new_value = get_active();
		if(new_value != m_data->value())
		{
			// Turn this into an undo/redo -able event ...
			if(m_data->state_recorder)
				m_data->state_recorder->start_recording(k3d::create_state_change_set(K3D_CHANGE_SET_CONTEXT), K3D_CHANGE_SET_CONTEXT);

			// Update everything with the new value ...
			m_data->set_value(new_value);

			// Turn this into an undo/redo -able event ...
			if(m_data->state_recorder)
				m_data->state_recorder->commit_change_set(m_data->state_recorder->stop_recording(K3D_CHANGE_SET_CONTEXT), new_value ? m_data->change_message + " \"On\"" : m_data->change_message + " \"Off\"", K3D_CHANGE_SET_CONTEXT);
		}
	}

	base::on_toggled();
}

} // namespace check_button

} // namespace ngui

} // namespace k3d

