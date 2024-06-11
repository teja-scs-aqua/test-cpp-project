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

#include <k3dsdk/ngui/file_chooser_dialog.h>
#include <k3dsdk/ngui/hotkey_entry.h>
#include <k3dsdk/ngui/path_chooser.h>

#include <k3d-i18n-config.h>
#include <k3dsdk/hints.h>
#include <k3dsdk/istate_recorder.h>
#include <k3dsdk/iwatched_path_property.h>
#include <k3dsdk/path.h>
#include <k3dsdk/result.h>
#include <k3dsdk/share.h>
#include <k3dsdk/state_change_set.h>
#include <k3dsdk/string_cast.h>

#include <gtkmm/combobox.h>
#include <gtkmm/entry.h>
#include <gtkmm/liststore.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/tooltips.h>

namespace k3d
{

namespace ngui
{

namespace path_chooser
{

/// Specialization of k3d::path_chooser::data_proxy for use with k3d::iproperty objects
template<>
class data_proxy<k3d::iproperty> :
	public idata_proxy
{
public:
	typedef k3d::iproperty data_t;

	data_proxy(data_t& Data, k3d::istate_recorder* const StateRecorder, const Glib::ustring& ChangeMessage) :
		idata_proxy(StateRecorder, ChangeMessage),
		m_readable_data(Data),
		m_writable_data(dynamic_cast<k3d::iwritable_property*>(&Data)),
		m_path_data(dynamic_cast<k3d::ipath_property*>(&Data)),
		m_watched_data(dynamic_cast<k3d::iwatched_path_property*>(&Data))
	{
	}

	const k3d::ipath_property::mode_t mode()
	{
		return m_path_data ? m_path_data->property_path_mode() : k3d::ipath_property::READ;
	}

	const std::string type()
	{
		return m_path_data ? m_path_data->property_path_type() : "unknown";
	}

	const k3d::ipath_property::reference_t reference()
	{
		return m_path_data ? m_path_data->property_path_reference() : k3d::ipath_property::RELATIVE_REFERENCE;
	}

	void set_reference(const k3d::ipath_property::reference_t Reference)
	{
		return_if_fail(m_path_data);
		m_path_data->set_property_path_reference(Reference);
	}

	const k3d::ipath_property::pattern_filters_t pattern_filters()
	{
		return m_path_data ? m_path_data->pattern_filters() : k3d::ipath_property::pattern_filters_t();
	}

	const k3d::filesystem::path value()
	{
		return boost::any_cast<k3d::filesystem::path>(m_readable_data.property_internal_value());
	}

	void set_value(const k3d::filesystem::path& Value)
	{
		return_if_fail(m_writable_data);
		m_writable_data->property_set_value(Value);
	}

	changed_signal_t& changed_signal()
	{
		return m_readable_data.property_changed_signal();
	}
	
	/// True if the underlying path can be watched for changes
	const k3d::bool_t is_watchable() const
	{
		return m_watched_data;
	}
	
	/// True if the path is watchable and is actually being watched 
	const k3d::bool_t is_watched() const
	{
		return m_watched_data ? m_watched_data->is_watched() : false;
	}
	
	void watch(const k3d::bool_t Watched)
	{
		return_if_fail(m_watched_data);
		m_watched_data->watch(Watched);
	}

private:
	data_proxy(const data_proxy& RHS);
	data_proxy& operator=(const data_proxy& RHS);

	data_t& m_readable_data;
	k3d::iwritable_property* const m_writable_data;
	k3d::ipath_property* const m_path_data;
	k3d::iwatched_path_property* const m_watched_data;
};

std::unique_ptr<idata_proxy> proxy(k3d::iproperty& Data, k3d::istate_recorder* const StateRecorder, const Glib::ustring& ChangeMessage)
{
	return std::unique_ptr<idata_proxy>(new data_proxy<k3d::iproperty>(Data, StateRecorder, ChangeMessage));
}

/////////////////////////////////////////////////////////////////////////////
// control

control::control(std::unique_ptr<idata_proxy> Data) :
	base(false, 0),
	m_entry(new hotkey_entry),
	m_button(new Gtk::Button("...")),
	m_combo(new Gtk::ComboBox()),
	m_toggle_button(0),
	m_data(std::move(Data)),
	m_disable_set_value(false)
{
	m_entry->signal_focus_out_event().connect(sigc::mem_fun(*this, &control::on_focus_out_event));
	m_entry->signal_activate().connect(sigc::mem_fun(*this, &control::on_activate));

	m_button->set_tooltip_text(_("Browse for a file ..."));
	m_button->signal_clicked().connect(sigc::mem_fun(*this, &control::on_browse));

	Glib::RefPtr<Gtk::ListStore> model = Gtk::ListStore::create(m_columns);
	Gtk::TreeRow row;
	row = *model->append();
		row[m_columns.reference] = k3d::ipath_property::ABSOLUTE_REFERENCE;
		row[m_columns.label] = _("Absolute");
	row = *model->append();
		row[m_columns.reference] = k3d::ipath_property::RELATIVE_REFERENCE;
		row[m_columns.label] = _("Relative");
	row = *model->append();
		row[m_columns.reference] = k3d::ipath_property::INLINE_REFERENCE;
		row[m_columns.label] = _("Inline");

	m_combo->set_model(model);
	m_combo->pack_start(m_columns.label);
	m_combo->set_tooltip_text(_("Choose whether to store absolute or relative filepaths"));

	m_combo->signal_changed().connect(sigc::mem_fun(*this, &control::on_pick_reference_type));

	pack_start(*manage(m_entry), Gtk::PACK_EXPAND_WIDGET);
	pack_start(*manage(m_button), Gtk::PACK_SHRINK);
	pack_start(*manage(m_combo), Gtk::PACK_SHRINK);
	
	if(m_data.get() && m_data->is_watchable())
	{
		m_toggle_button = new Gtk::ToggleButton(_("Watch"), true);
		m_toggle_button->set_active(true);
		m_toggle_button->signal_toggled().connect(sigc::mem_fun(*this, &control::on_watch_toggle));
		m_toggle_button->set_tooltip_text(_("Watch/unwatch file for changes"));
		pack_start(*manage(m_toggle_button), Gtk::PACK_SHRINK);
	}

	data_changed(0);

	if(m_data.get())
		m_data->changed_signal().connect(sigc::mem_fun(*this, &control::data_changed));

	show_all();
}

bool control::on_focus_out_event(GdkEventFocus* Event)
{
	set_value();
	return false;
}

void control::on_activate()
{
	set_value();
}

void control::on_browse()
{
	return_if_fail(m_data.get());

	k3d::filesystem::path new_value;
	{
		file_chooser_dialog dialog("", m_data->type(), m_data->mode(), m_data->value());

		const k3d::ipath_property::pattern_filters_t filters = m_data->pattern_filters();
		for(k3d::ipath_property::pattern_filters_t::const_iterator filter = filters.begin(); filter != filters.end(); ++filter)
			dialog.add_pattern_filter(filter->name, filter->pattern);
		if(!filters.empty())
			dialog.add_all_files_filter();

		if(!dialog.get_file_path(new_value))
			return;
	}

	// Turn this into an undo/redo -able event ...
	if(m_data->state_recorder)
		m_data->state_recorder->start_recording(k3d::create_state_change_set(K3D_CHANGE_SET_CONTEXT), K3D_CHANGE_SET_CONTEXT);

	// Update everything with the new value ...
	m_data->set_value(new_value);

	// Turn this into an undo/redo -able event ...
	if(m_data->state_recorder)
		m_data->state_recorder->commit_change_set(m_data->state_recorder->stop_recording(K3D_CHANGE_SET_CONTEXT), m_data->change_message + " " + new_value.native_utf8_string().raw(), K3D_CHANGE_SET_CONTEXT);
}

void control::on_pick_reference_type()
{
	return_if_fail(m_data.get());
	return_if_fail(m_combo->get_active() != m_combo->get_model()->children().end());

	m_data->set_reference(m_combo->get_active()->get_value(m_columns.reference));
}

void control::on_watch_toggle()
{
	if(m_disable_set_value)
		return;
	return_if_fail(m_data.get());
	m_data->watch(!m_data->is_watched());
}

void control::set_value()
{
	if(m_disable_set_value)
		return;

	return_if_fail(m_data.get());

	m_disable_set_value = true;

	k3d::filesystem::path new_value;
	try
	{
		new_value = k3d::filesystem::native_path(k3d::ustring::from_utf8(m_entry->get_text()));
	}
	catch(std::exception& e)
	{
		k3d::log() << error << e.what() << std::endl;
		m_disable_set_value = false;
		return;
	}

	// If the value didn't change, we're done ...
	if(m_data->value().generic_utf8_string() == new_value.generic_utf8_string())
	{
		m_disable_set_value = false;
		return;
	}

	// Turn this into an undo/redo -able event ...
	if(m_data->state_recorder)
		m_data->state_recorder->start_recording(k3d::create_state_change_set(K3D_CHANGE_SET_CONTEXT), K3D_CHANGE_SET_CONTEXT);

	// Update everything with the new value ...
	m_data->set_value(new_value);

	// Turn this into an undo/redo -able event ...
	if(m_data->state_recorder)
		m_data->state_recorder->commit_change_set(m_data->state_recorder->stop_recording(K3D_CHANGE_SET_CONTEXT), m_data->change_message + " " + new_value.native_utf8_string().raw(), K3D_CHANGE_SET_CONTEXT);

	m_disable_set_value = false;
}

void control::data_changed(k3d::ihint* Hint)
{
	return_if_fail(m_data.get());

	m_entry->set_text(m_data->value().leaf().raw());
	m_entry->set_tooltip_text(m_data->value().native_utf8_string().raw());

	m_combo->set_active(m_data->reference());
	if(m_toggle_button)
	{
		m_disable_set_value = true;
		m_toggle_button->set_active(m_data->is_watched());
		m_disable_set_value = false;
	}
}

void control::on_reference_type_changed()
{
	m_combo->set_active(m_data->reference());
}

} // namespace path_chooser

} // namespace ngui

} // namespace k3d

