// K-3D
// Copyright (c) 1995-2007, Timothy M. Shead
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
		\brief A track object, containing keyframes of an animation
		\author Bart Janssens (bart.janssens@lid.kviv.be)
*/

#include <k3dsdk/algebra.h>
#include <k3dsdk/ikeyframer.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/node.h>
#include <k3dsdk/property_group_collection.h>
#include <k3dsdk/string_cast.h>
#include <k3dsdk/tokens.h>
#include <k3dsdk/value_demand_storage.h>
#include <k3dsdk/vectors.h>

#include "interpolator.h"

namespace module
{

namespace animation
{

template <typename time_t, typename value_t> class animation_track;

/// State container that restores using a "store keyframe" operation
template <typename time_t, typename value_t>
class store_state_container :
	public k3d::istate_container
{
public:
	store_state_container(animation_track<time_t, value_t>& Track, time_t Time, value_t Value, const std::string& Keynumber) :
		m_track(Track),
		m_time(Time),
		m_value(Value),
		m_keynumber(Keynumber)
	{}
	
	void restore_state()
	{
		m_track.store_value(m_time, m_value, m_keynumber);
		m_track.reset_output();
	}
	
	void update_value(const value_t& Value)
	{
		m_value = Value;
	}
	
private:
	animation_track<time_t, value_t>& m_track;
	time_t m_time;
	value_t m_value;
	std::string m_keynumber;
};

/// State container that restores using a delete_key operation
template <typename time_t, typename value_t>
class delete_state_container :
	public k3d::istate_container
{
public:
	delete_state_container(animation_track<time_t, value_t>& Track, const std::string& Keynumber) :
		m_track(Track),
		m_keynumber(Keynumber)
	{}
	
	void restore_state()
	{
		m_track.delete_key(m_keynumber);
	}
	
private:
	animation_track<time_t, value_t>& m_track;
	std::string m_keynumber;
};

/// State container that updates an existing key
template <typename time_t, typename value_t>
class update_state_container :
	public k3d::istate_container
{
public:
	update_state_container(animation_track<time_t, value_t>& Track, const std::string& Label, const value_t& Value) :
		m_track(Track),
		m_label(Label),
		m_value(Value)
	{}
	
	void restore_state()
	{
		m_track.update_value(m_label, m_value);
	}
	
private:
	animation_track<time_t, value_t>& m_track;
	std::string m_label;
	value_t m_value;
};

/// Encapsulates a series of keyframes
template <typename time_t, typename value_t>
class animation_track :
	public k3d::node,
	public k3d::property_group_collection,
	public k3d::ikeyframer
{
	typedef k3d::node base;
	typedef k3d_data(time_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) time_property_t;
	typedef k3d_data(value_t, immutable_name, change_signal, no_undo, local_storage, no_constraint, writable_property, with_serialization) value_property_t;
	typedef std::map<time_property_t*, value_property_t*> keyframes_t;
	typedef interpolator<time_t, value_t> interpolator_t;
	typedef std::map<time_property_t*, std::string> keygroups_t;
public:
	animation_track(k3d::iplugin_factory& Factory, k3d::idocument& Document, time_t Time, value_t Value) :
		base(Factory, Document),
		m_time_input(init_owner(*this) + init_name("time_input") + init_label(("Time Input")) + init_description(("Time for the animation")) + init_value(Time)),
		m_value_input(init_owner(*this) + init_name("value_input") + init_label(("Value Input")) + init_description(("Input that is keyframed when it changes")) + init_value(Value)),
		m_output_value(init_owner(*this) + init_name("output_value") + init_label(("Output Value")) + init_description(("Interpolated output value")) + init_value(value_t())),
		m_interpolator(init_owner(*this) + init_name("interpolator") + init_label("Interpolator") + init_description("Method used to interpolate keyframes") + init_value(static_cast<interpolator_t*>(0))),
		m_manual_keyframe(init_owner(*this) + init_name("manual_keyframe") + init_label(("Manual keyframe only")) + init_description(("If checked, keyframes are created only usint the timeline. Otherwise keyframes are created/updated whenever the Value Input changes")) + init_value(false)),
		m_record(true),
		m_no_interpolation(false)
	{
		m_output_value.set_update_slot(sigc::mem_fun(*this, &animation_track::on_output_request));
		m_time_input.changed_signal().connect(m_output_value.make_slot());
		m_value_input.changed_signal().connect(sigc::mem_fun(*this, &animation_track::on_value_change));
	}
	
	void on_output_request(const std::vector<k3d::ihint*>& Hints, value_t& Output)
	{
		// If we're not supposed to interpolate, return the input
		if (m_no_interpolation)
		{
			m_no_interpolation = false;
			Output = m_value_input.pipeline_value();
			return;
		}
		// Interpolate value
		interpolator_t* interpolator = m_interpolator.pipeline_value();
		if (!interpolator)
		{
			Output =  m_value_input.pipeline_value();
			return;
		}
		typename interpolator_t::keyframes_t keyframes;
		for (typename keyframes_t::iterator keyframe = m_keyframes.begin(); keyframe != m_keyframes.end(); ++keyframe)
			keyframes.insert(std::make_pair(keyframe->first->pipeline_value(), keyframe->second->pipeline_value()));
		time_t time = m_time_input.pipeline_value();
		try
		{
			Output = interpolator->interpolate(time, keyframes);
		}
		catch (insufficient_data_exception& e)
		{
			k3d::log() << warning << name() << ": Insufficient keyframe data to calculate value at time " << time << std::endl;
			Output =  m_value_input.pipeline_value();
		}
	}
	
	/// Create a keyframe from the current time and value inputs
	void keyframe()
	{
		time_t time = m_time_input.pipeline_value();
		value_t value = m_value_input.pipeline_value();
		time_property_t* time_property = 0;
		for (typename keyframes_t::const_iterator keyframe = m_keyframes.begin(); keyframe != m_keyframes.end(); ++keyframe)
		{
			if (keyframe->first->pipeline_value() == time)
			{
				time_property = keyframe->first;
				break;
			}
		}
		typename keyframes_t::iterator value_it;
		// Create new keyframe if a keyframe did not already exist for the current time
		if (time_property == 0)
		{
			std::string key_number = boost::lexical_cast<std::string>(m_keyframes.size());
			store_value(time, value, key_number);
			// Make keyframe creation undoable
			if (!document().state_recorder().current_change_set())
			{
				k3d::record_state_change_set changeset(document(), "Create keyframe " + key_number, K3D_CHANGE_SET_CONTEXT);
				record_create(time, value, key_number);
			}
			else
			{
				record_create(time, value, key_number);
			}
		}
		else
		{
			value_it = m_keyframes.find(time_property);
			// record undo/redo
			if(document().state_recorder().current_change_set())
			{
				document().state_recorder().current_change_set()->record_old_state(new update_state_container<time_t, value_t>(*this, value_it->second->property_label(), value_it->second->internal_value()));
				document().state_recorder().current_change_set()->record_new_state(new update_state_container<time_t, value_t>(*this, value_it->second->property_label(), value));
			}
			value_it->second->set_value(value);
			// Update the stored key creation redo value in case we are creating a key through a drag motion
			if (m_last_set == document().state_recorder().current_change_set())
			{
				m_last_store->update_value(value);
			}
			else
			{
				m_last_set = 0;
				m_last_store = 0;
			}
		}
	}
	
	void reset_output(bool Interpolate = true)
	{
		m_no_interpolation = !Interpolate;
		m_output_value.update();
	}
	
	void delete_key(k3d::iproperty* TimeProperty)
	{
		record_delete(TimeProperty);
		
		time_property_t* time_property = dynamic_cast<time_property_t*>(TimeProperty);
		value_property_t* value_property = m_keyframes[time_property];
		unregister_property_group(m_keygroups[time_property]);
		disable_serialization(*value_property);
		disable_serialization(*time_property);
		unregister_property(*value_property);
		unregister_property(*time_property);
		m_keyframes.erase(time_property);
		m_keygroups.erase(time_property);
		delete value_property;
		delete time_property;
		m_keys_changed_signal.emit();
		reset_output();
	}
	
	/// Delete key by number (used for redo)
	void delete_key(const std::string& Keynumber)
	{
		m_record = false; // don't record this delete
		for (typename keyframes_t::const_iterator keyframe = m_keyframes.begin(); keyframe != m_keyframes.end(); ++keyframe)
		{
			if (keyframe->first->property_name().substr(9, keyframe->first->property_name().size() - 8) == Keynumber)
			{
				delete_key(keyframe->first);
				break;
			}
		}
		m_record = true;
	}
	
	k3d::iproperty& input_property()
	{
		return m_value_input;
	}
	
	k3d::ikeyframer::keys_t get_keys()
	{
		k3d::ikeyframer::keys_t keys;
		for (typename keyframes_t::const_iterator keyframe = m_keyframes.begin(); keyframe != m_keyframes.end(); ++keyframe)
			keys.push_back(keyframe->first);
		return keys;
	}
		
	/// Make sure the keyframe structures get updated on load
	void load(k3d::xml::element& Element, const k3d::ipersistent::load_context& Context)
	{
		base::load(Element, Context);
		
		k3d::xml::element* const properties = k3d::xml::find_element(Element, "properties");
		return_if_fail(properties);
		
		// Keep track of frames by stored number
		std::map<std::string, time_t> times;
		
		// Read the times (this is not an ad)
		for(k3d::xml::element::elements_t::iterator element = properties->children.begin(); element != properties->children.end(); ++element)
		{
			if(element->name == "property")
			{
				const std::string& property_name = k3d::xml::attribute_value<std::string>(*element, "name", "");
				if (property_name.find("key_time_") == 0)
				{
					std::string keynumber = property_name.substr(9, property_name.size() - 8);
					time_t time = k3d::from_string<time_t>(element->text, time_t());
					times[keynumber] = time;
				}
			}
		}
		
		// Read the values and create the properties
		for(k3d::xml::element::elements_t::iterator element = properties->children.begin(); element != properties->children.end(); ++element)
		{
			if(element->name == "property")
			{
				const std::string& property_name = k3d::xml::attribute_value<std::string>(*element, "name", "");
				if (property_name.find("key_value_") == 0)
				{
					std::string keynumber = property_name.substr(10, property_name.size() - 9);
					value_t value = k3d::from_string<value_t>(element->text, value_t());
					typename std::map<std::string, time_t>::iterator time_it = times.find(keynumber);
					return_if_fail(time_it != times.end());
					store_value(time_it->second, value, keynumber);
				}
			}
		}
	}
	
	/// Stores the given value keyed by the given time property, and handles group creation
	void store_value(time_t Time, value_t Value, const std::string& KeyNumber)
	{
		std::string time_name = "key_time_" + KeyNumber;
		std::string time_label = "Key Time " + KeyNumber;	
		std::string value_name = "key_value_" + KeyNumber;
		std::string value_label = "Key Value " + KeyNumber;
		std::string group_name = "Key " + KeyNumber;
		std::string delete_name = "delete_" + KeyNumber;
		std::string delete_label = "Delete Key " + KeyNumber;
		
		// time property
		time_property_t* time_property = new time_property_t(init_owner(*this) + init_name(k3d::make_token(time_name.c_str())) + init_label(k3d::make_token(time_label.c_str())) + init_description(("")) + init_value(Time));
		
		// value property
		typename keyframes_t::iterator value_it = m_keyframes.insert(std::make_pair(time_property, new value_property_t(init_owner(*this) + init_name(k3d::make_token(value_name.c_str())) + init_label(k3d::make_token(value_label.c_str())) + init_description(("")) + init_value(Value)))).first;
		
		// group for this key
		k3d::iproperty_group_collection::group key_group(group_name);
		m_keygroups[time_property] = group_name;
		key_group.properties.push_back(static_cast<k3d::iproperty*>(time_property));
		key_group.properties.push_back(static_cast<k3d::iproperty*>(value_it->second));
		register_property_group(key_group);
		m_keys_changed_signal.emit();
	}
	
	/// Stores the current value at Time using keynumber
	void store_current_value(time_t Time, const std::string& KeyNumber)
	{
		store_value(Time, m_value_input.pipeline_value(), KeyNumber);
	}
	
	/// Updates the value of the property named by label
	void update_value(const std::string& Label, const value_t& Value)
	{
		for (typename keyframes_t::iterator keyframe = m_keyframes.begin(); keyframe != m_keyframes.end(); ++keyframe)
		{
			if (keyframe->second->property_label() == Label)
			{
				keyframe->second->set_value(Value);
				return;
			}
		}
		k3d::log() << warning << "animation_track: Labeled value property not found!" << std::endl;
	}
	
	k3d::ikeyframer::keys_changed_signal_t& keys_changed_signal()
	{
		return m_keys_changed_signal;
	}
	

private:
	
	/// Executed when the input value changes
	void on_value_change(k3d::ihint* Hint)
	{
		bool manual_key = m_manual_keyframe.pipeline_value();
		if(!manual_key && document().state_recorder().current_change_set())
		{
			keyframe();
		}
		reset_output(!manual_key);
	}
	
	/// Record delete undo
	void record_delete(k3d::iproperty* TimeProperty)
	{
		if (!m_record)
			return;

		time_property_t* time_property = dynamic_cast<time_property_t*>(TimeProperty);
		value_property_t* value_property = m_keyframes[time_property];
		std::string keynumber = time_property->property_name().substr(9, time_property->property_name().size() - 8);
		
		k3d::record_state_change_set changeset(document(), "Delete keyframe " + keynumber, K3D_CHANGE_SET_CONTEXT);
		
		if(document().state_recorder().current_change_set())
		{
			document().state_recorder().current_change_set()->record_old_state(new store_state_container<time_t, value_t>(*this, time_property->pipeline_value(), value_property->pipeline_value(), keynumber));
			document().state_recorder().current_change_set()->record_new_state(new delete_state_container<time_t, value_t>(*this, keynumber));
		}
	}
	
	/// Record key creation undo
	void record_create(time_t& Time, value_t& Value, const std::string& Keynumber)
	{
		if (document().state_recorder().current_change_set())
		{
			m_last_store = new store_state_container<time_t, value_t>(*this, Time, Value, Keynumber);
			m_last_set = document().state_recorder().current_change_set();
			document().state_recorder().current_change_set()->record_new_state(m_last_store);
			document().state_recorder().current_change_set()->record_old_state(new delete_state_container<time_t, value_t>(*this, Keynumber));
		}
	}

	time_property_t m_time_input;
	value_property_t m_value_input;
	k3d_data(value_t, k3d::data::immutable_name, k3d::data::change_signal, k3d::data::no_undo, k3d::data::value_demand_storage, k3d::data::no_constraint, k3d::data::read_only_property, k3d::data::no_serialization) m_output_value;
	k3d_data(interpolator_t*, k3d::data::immutable_name, k3d::data::change_signal, k3d::data::with_undo, k3d::data::node_storage, k3d::data::no_constraint, k3d::data::node_property, k3d::data::node_serialization) m_interpolator;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_manual_keyframe;
	keyframes_t m_keyframes;
	keygroups_t m_keygroups;
	/// True if changes should be recorded for undo/redo
	bool m_record;
	/// If true, interpolation is turned off and the input value is passed on as output
	bool m_no_interpolation;
	/// Last state change set that was being recorded to
	k3d::state_change_set* m_last_set; 
	store_state_container<time_t, value_t>* m_last_store;
	k3d::ikeyframer::keys_changed_signal_t m_keys_changed_signal;
};

/////////////////////////////////////////////////////////////////////////////
// specific instances and factories

////////// matrix4 track /////////////////:

class animation_track_double_matrix4 : public animation_track<double, k3d::matrix4>
{
	typedef animation_track<double, k3d::matrix4> base;
public:
	animation_track_double_matrix4(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document, 0.0, k3d::identity3()) {}
	
	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<animation_track_double_matrix4, k3d::interface_list<k3d::ikeyframer> >factory(
				k3d::uuid(0x00347e9b, 0x97486a2b, 0xb79e71ab, 0xc719354f),
				"AnimationTrackDoubleMatrix4",
				("Stores a series of keyframes for an animation, using 'double' as time type and 'matrix4' as value"),
				"Animation",
				k3d::iplugin_factory::EXPERIMENTAL);
		return factory;
	}
};

k3d::iplugin_factory& animation_track_double_matrix4_factory()
{
	return animation_track_double_matrix4::get_factory();
}

/////////////// double track ///////////

class animation_track_double_double : public animation_track<double, double>
{
	typedef animation_track<double, double> base;
public:
	animation_track_double_double(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document, 0.0, 0.0) {}
	
	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<animation_track_double_double, k3d::interface_list<k3d::ikeyframer> >factory(
				k3d::uuid(0xa0b9d507, 0x20400293, 0x8f13c393, 0x31d908a8),
				"AnimationTrackDoubleDouble",
				("Stores a series of keyframes for an animation, using 'double' as time and value"),
				"Animation",
				k3d::iplugin_factory::EXPERIMENTAL);
		return factory;
	}
};

k3d::iplugin_factory& animation_track_double_double_factory()
{
	return animation_track_double_double::get_factory();
}

} // namespace animation

} // namespace module
