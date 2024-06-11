// K-3D
// Copyright (c) 1995-2006, Timothy M. Shead
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

#include <k3dsdk/fstream.h>
#include <k3d-i18n-config.h>
#include <k3dsdk/istate_container.h>
#include <k3dsdk/itexture_ri.h>
#include <k3dsdk/iuser_property.h>
#include <k3dsdk/options.h>
#include <k3dsdk/property.h>
#include <k3dsdk/shader_ri.h>
#include <k3dsdk/user_property.h>

namespace k3d
{

namespace ri
{

namespace detail
{

const std::string shader_type_path(const k3d::sl::shader::type_t ShaderType)
{
	switch(ShaderType)
	{
		case k3d::sl::shader::SURFACE:
			return k3d::options::path::surface_shaders();
		case k3d::sl::shader::LIGHT:
			return k3d::options::path::light_shaders();
		case k3d::sl::shader::VOLUME:
			return k3d::options::path::volume_shaders();
		case k3d::sl::shader::DISPLACEMENT:
			return k3d::options::path::displacement_shaders();
		case k3d::sl::shader::TRANSFORMATION:
			return k3d::options::path::transformation_shaders();
		case k3d::sl::shader::IMAGER:
			return k3d::options::path::imager_shaders();
	}

	assert_not_reached();
	return std::string();
}

} // namespace detail

/////////////////////////////////////////////////////////////////////////////
// shader

shader::shader(iplugin_factory& Factory, idocument& Document, k3d::sl::shader::type_t ShaderType) :
	base(Factory, Document),
	m_shader_type(ShaderType),
	m_shader_path(init_owner(*this) + init_name("shader_path") + init_label(_("Shader Path")) + init_description(_("Shader Path")) + init_value(filesystem::path()) + init_path_mode(k3d::ipath_property::READ) + init_path_type(detail::shader_type_path(ShaderType))),
	m_shader(init_owner(*this) + init_value(sl::shader(ShaderType))),
	m_user_property_changed_signal(*this)
{
	m_shader_path.add_pattern_filter(ipath_property::pattern_filter(_("RenderMan shader (*.sl)"), "*.sl"));
	m_shader_connection = m_shader_path.connect_explicit_change_signal(sigc::mem_fun(*this, &shader::on_shader_changed));

	m_user_property_changed_signal.connect(make_node_change_slot());
}

shader::~shader()
{
}

void shader::load(xml::element& Element, const ipersistent::load_context& Context)
{
	// Disable argument list updates while loading so we don't create the same arguments twice
	m_shader_connection.disconnect();
	base::load(Element, Context);
	m_shader_connection = m_shader_path.connect_explicit_change_signal(sigc::mem_fun(*this, &shader::on_shader_changed));

	load_metafile();
}

const path shader::shader_path()
{
	return m_shader_path.pipeline_value();
}

const std::string shader::shader_name()
{
	return m_shader.internal_value().name;
}

parameter_list shader::shader_arguments(const render_state& State)
{
	parameter_list results;

	const iproperty_collection::properties_t& properties = base::properties();
	for(iproperty_collection::properties_t::const_iterator prop = properties.begin(); prop != properties.end(); ++prop)
	{
		if(dynamic_cast<iuser_property*>(*prop))
		{
			iproperty& property = **prop;
			const std::type_info& property_type = property.property_type();

			if(property_type == typeid(ri::real))
			{
				results.push_back(parameter(property.property_name(), CONSTANT, 1, property::pipeline_value<ri::real>(property)));
			}
			else if(property_type == typeid(ri::string))
			{
				results.push_back(parameter(property.property_name(), CONSTANT, 1, property::pipeline_value<ri::string>(property)));
			}
			else if(property_type == typeid(k3d::inode*)) // Node properties always are of type "inode*", so we have to query for the interface type we really want
			{
				if(k3d::ri::itexture* const texture = dynamic_cast<k3d::ri::itexture*>(property::pipeline_value<inode*>(property)))
					results.push_back(parameter(property.property_name(), CONSTANT, 1, static_cast<k3d::ri::string>(texture->renderman_texture_path(State).native_filesystem_string())));
			}
			else if(property_type == typeid(ri::point))
			{
				results.push_back(parameter(property.property_name(), CONSTANT, 1, property::pipeline_value<ri::point>(property)));
			}
			else if(property_type == typeid(ri::vector))
			{
				results.push_back(parameter(property.property_name(), CONSTANT, 1, property::pipeline_value<ri::vector>(property)));
			}
			else if(property_type == typeid(ri::normal))
			{
				results.push_back(parameter(property.property_name(), CONSTANT, 1, property::pipeline_value<ri::normal>(property)));
			}
			else if(property_type == typeid(ri::hpoint))
			{
				results.push_back(parameter(property.property_name(), CONSTANT, 1, property::pipeline_value<ri::hpoint>(property)));
			}
			else if(property_type == typeid(ri::matrix))
			{
				results.push_back(parameter(property.property_name(), CONSTANT, 1, property::pipeline_value<ri::matrix>(property)));
			}
			else if(property_type == typeid(ri::color))
			{
				results.push_back(parameter(property.property_name(), CONSTANT, 1, property::pipeline_value<ri::color>(property)));
			}
			else
			{
				log() << error << k3d_file_reference << ": unknown storage type [" << property_type.name() << "] for shader argument [" << property.property_name() << "] will be ignored" << std::endl;
			}
		}
	}

	return results;
}

void shader::on_shader_changed(iunknown*)
{
	if(document().state_recorder().current_change_set())
		document().state_recorder().current_change_set()->record_old_state(new user::property_container(*this));

	delete_arguments();
	load_metafile();
	create_arguments();

	make_node_change_slot()(0);

	if(document().state_recorder().current_change_set())
		document().state_recorder().current_change_set()->record_new_state(new user::property_container(*this));
}

void shader::delete_arguments()
{
	const iproperty_collection::properties_t user_properties = k3d::property::user_properties(*this);

	unregister_properties(user_properties);

	for(iproperty_collection::properties_t::const_iterator property = user_properties.begin(); property != user_properties.end(); ++property)
	{
		if(ipersistent* const persistent = dynamic_cast<ipersistent*>(*property))
			disable_serialization(*persistent);
	}

	for(iproperty_collection::properties_t::const_iterator property = user_properties.begin(); property != user_properties.end(); ++property)
		undoable_delete(*property, document());
}

void shader::load_metafile()
{
    try
    {
        const filesystem::path shader_path = m_shader_path.pipeline_value();
        const filesystem::path metafile_path = shader_path + ".slmeta";
        filesystem::ifstream metafile_stream(metafile_path);

        const sl::shaders_t shaders = sl::parse_metafile(metafile_stream, shader_path, metafile_path);
        if(shaders.size() != 1)
        {
            log() << error << "Can't load metafile describing shader [" << shader_path.native_console_string() << "]" << std::endl;
            return;
        }

        const sl::shaders_t::const_iterator shader = shaders.begin();
        if(shader->type != m_shader_type)
        {
            log() << error << "Shader [" << shader_path.native_console_string() << "] is not the correct shader type" << std::endl;
            return;
        }

        m_shader.set_value(*shader);
    }
    catch(std::exception& e)
    {
        log() << error << "Exception loading shader metafile: " << e.what() << std::endl;
    }
}

void shader::create_arguments()
{
	const sl::shader shader = m_shader.internal_value();
	for(sl::shader::arguments_t::const_iterator argument = shader.arguments.begin(); argument != shader.arguments.end(); ++argument)
	{
		if(argument->output)
			continue;

		const string_t& name = argument->name;
		const string_t& label = argument->label;
		const string_t& description = argument->description;
		const string_t& default_value = argument->default_value;

		switch(argument->extended_type)
		{
			case sl::argument::EX_FLOAT:
			case sl::argument::EX_TIME:
			case sl::argument::EX_ANGLE:
			case sl::argument::EX_DISTANCE:
			case sl::argument::EX_AREA:
			case sl::argument::EX_VOLUME:
			case sl::argument::EX_MASS:
			case sl::argument::EX_FORCE:
			case sl::argument::EX_PRESSURE:
			{
				undoable_new(property::create<double_t>(*this, name, label, description, from_string<double>(default_value, 0.0)), document());
				break;
			}
			case sl::argument::EX_STRING:
			case sl::argument::EX_SPACE:
			{
				undoable_new(property::create<string_t>(*this, name, label, description, default_value), document());
				break;
			}
			case sl::argument::EX_TEXTURE:
			{
				undoable_new(property::create<ri::itexture*>(*this, name, label, description, static_cast<k3d::ri::itexture*>(0)), document());
				break;
			}
			case sl::argument::EX_POINT:
			{
				undoable_new(property::create<point3>(*this, name, label, description, from_string<point3>(default_value, point3(0, 0, 0))), document());
				break;
			}
			case sl::argument::EX_VECTOR:
			{
				undoable_new(property::create<vector3>(*this, name, label, description, from_string<vector3>(default_value, vector3(0, 0, 0))), document());
				break;
			}
			case sl::argument::EX_NORMAL:
			{
				undoable_new(property::create<normal3>(*this, name, label, description, from_string<normal3>(default_value, normal3(0, 0, 0))), document());
				break;
			}
			case sl::argument::EX_HPOINT:
			{
				undoable_new(property::create<point4>(*this, name, label, description, from_string<point4>(default_value, point4(0, 0, 0, 0))), document());
				break;
			}
			case sl::argument::EX_MATRIX:
			{
				undoable_new(property::create<matrix4>(*this, name, label, description, from_string<matrix4>(default_value, matrix4())), document());
				break;
			}
			case sl::argument::EX_COLOR:
			{
				undoable_new(property::create<color>(*this, name, label, description, from_string<color>(default_value, color(1, 1, 1))), document());
				break;
			}
			default:
			{
				log() << error << k3d_file_reference << " unknown extended argument type for [" << argument->name << "] will not receive storage" << std::endl;
			}
		}
	}
}

} // namespace ri

} // namespace k3d

