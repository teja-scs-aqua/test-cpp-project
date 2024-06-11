#ifndef K3DSDK_WATCHED_PATH_PROPERTY_H
#define K3DSDK_WATCHED_PATH_PROPERTY_H

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

/** \file watched_path_property.h
		\author Bart Janssens (bart.janssens@lid.kviv.be)
		\created Feb 6, 2009
*/

#include <k3dsdk/data.h>
#include <k3dsdk/hints.h>
#include <k3dsdk/ihint.h>
#include <k3dsdk/iuser_interface.h>
#include <k3dsdk/iwatched_path_property.h>
#include <k3dsdk/path.h>
#include <k3dsdk/types.h>

namespace k3d
{

template<typename value_t, class name_policy_t>
class watched_path_property :
	public iwatched_path_property,
	public path_property<value_t, name_policy_t>
{
	typedef path_property<value_t, name_policy_t> base;
public:
	void watch(const bool_t Watch)
	{
		m_watched = Watch;
		base::changed_signal().emit(0);
	}
	
	const bool_t is_watched() const
	{
		return m_watched;
	}

protected:
	template<typename init_t>
	watched_path_property(const init_t& Init) : base(Init), m_watched(true), m_watch_id(0)
	{
		base::changed_signal().connect(hint::converter<
			hint::convert<hint::any, hint::unchanged> >(sigc::mem_fun(*this, &watched_path_property::on_path_changed)));
	}
	
	~watched_path_property()
	{
		user_interface().unwatch_path(m_watch_id);
	}
	
private:
	void on_path_changed(ihint* Hint)
	{
		if(dynamic_cast<hint::file_changed*>(Hint))
		{
			return;
		}
		
		user_interface().unwatch_path(m_watch_id);
		m_watch_id = 0;
		
		const filesystem::path path = base::pipeline_value();
		if(path.empty() || !m_watched)
		{
			return;
		}
		
		m_watch_id = k3d::user_interface().watch_path(path, sigc::mem_fun(*this, &watched_path_property::on_file_changed));
	}
	
	void on_file_changed()
	{
		base::changed_signal().emit(hint::file_changed::instance());
	}
	
	bool_t m_watched;
	uint_t m_watch_id;	
};
	
/// Serialization policy for filesystem path data that handles external filesystem resources
template<typename value_t, class property_policy_t>
class watched_path_serialization :
	public path_serialization<value_t, property_policy_t>
{
	// This policy only works for data stored by-value
	BOOST_STATIC_ASSERT((!boost::is_pointer<value_t>::value));

	typedef path_serialization<value_t, property_policy_t> base;
	
public:
	void save(xml::element& Element, const ipersistent::save_context& Context)
	{
		xml::element& xml_storage = save_external_resource(Element, Context, property_policy_t::name(), property_policy_t::property_path_reference(), property_policy_t::internal_value());
		xml_storage.append(xml::attribute("watched", property_policy_t::is_watched()));
	}

	void load(xml::element& Element, const ipersistent::load_context& Context)
	{
		base::load(Element, Context);
		property_policy_t::watch(xml::attribute_value<bool_t>(Element, "watched", false));
	}

protected:
	template<typename init_t>
	watched_path_serialization(const init_t& Init) : base(Init) {}
};

} // namespace k3d

#endif // !K3DSDK_WATCHED_PATH_PROPERTY_H

